// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/cut_update_pool.h>

namespace lamure
{

namespace ren
{

CutUpdatePool::
CutUpdatePool(const context_t context_id,
  const node_t upload_budget_in_nodes,
  const node_t render_budget_in_nodes)
  : context_id_(context_id),
    locked_(false),
    num_threads_(LAMURE_CUT_UPDATE_NUM_CUT_UPDATE_THREADS),
    shutdown_(false),
    current_gpu_storage_A_(nullptr),
    current_gpu_storage_B_(nullptr),
    current_gpu_storage_(nullptr),
    current_gpu_buffer_(CutDatabaseRecord::TemporaryBuffer::BUFFER_A),
    upload_budget_in_nodes_(upload_budget_in_nodes),
    render_budget_in_nodes_(render_budget_in_nodes),
#ifdef LAMURE_CUT_UPDATE_ENABLE_MODEL_TIMEOUT
    cut_update_counter_(0),
#endif
    master_dispatched_(false) {

    Initialize();

    for (uint32_t i = 0; i < num_threads_; ++i) {
        threads_.push_back(std::thread(&CutUpdatePool::Run, this));
    }
}

CutUpdatePool::
~CutUpdatePool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        shutdown_ = true;
        semaphore_.Shutdown();
        master_semaphore_.Shutdown();
    }

    for (auto& thread : threads_) {
        if (thread.joinable())
            thread.join();
    }
    threads_.clear();

    Shutdown();
}

void CutUpdatePool::
Initialize() {
    ModelDatabase* database = ModelDatabase::GetInstance();
    Policy* policy = Policy::GetInstance();

    assert(policy->render_budget_in_mb() > 0);
    assert(policy->out_of_core_budget_in_mb() > 0);

    size_t size_of_node_in_bytes = database->size_of_surfel() * database->surfels_per_node();

    out_of_core_budget_in_nodes_ =
        (policy->out_of_core_budget_in_mb()*1024*1024) / size_of_node_in_bytes;

    index_ = new CutUpdateIndex();
    index_->UpdatePolicy(0);
    gpu_cache_ = new GpuCache(render_budget_in_nodes_);

    semaphore_.set_max_signal_count(1);
    semaphore_.set_min_signal_count(1);

#ifdef LAMURE_ENABLE_INFO
    std::cout << "PLOD: num models: " << index_->num_models() << std::endl;
    std::cout << "PLOD: ooc-cache size (MB): " << policy->out_of_core_budget_in_mb() << std::endl;
#endif

}

void CutUpdatePool::
Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (gpu_cache_ != nullptr) {
        delete gpu_cache_;
        gpu_cache_ = nullptr;
    }

    if (index_ != nullptr) {
        delete index_;
        index_ = nullptr;
    }

    current_gpu_storage_A_ = nullptr;
    current_gpu_storage_B_ = nullptr;

}

bool CutUpdatePool::
IsShutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    return shutdown_;
}

const bool CutUpdatePool::
IsRunning() {
    std::lock_guard<std::mutex> lock(mutex_);
    return master_dispatched_;
}

void CutUpdatePool::
DispatchCutUpdate(char* current_gpu_storage_A, char* current_gpu_storage_B) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(current_gpu_storage_A != nullptr);
    assert(current_gpu_storage_B != nullptr);

#ifdef LAMURE_CUT_UPDATE_ENABLE_REPEAT_MODE
    master_timer_.stop();
    boost::timer::cpu_times const last_frame_time(master_timer_.elapsed());
    last_frame_elapsed_ = last_frame_time.system + last_frame_time.user;
    master_timer_.start();
#endif

    if (!master_dispatched_) {
        current_gpu_storage_A_ = current_gpu_storage_A;
        current_gpu_storage_B_ = current_gpu_storage_B;

        master_dispatched_ = true;

        job_queue_.PushJob(
            CutUpdateQueue::Job(
                CutUpdateQueue::Task::CUT_MASTER_TASK, invalid_view_t, invalid_model_t));

        semaphore_.Signal(1);

    }

}


void CutUpdatePool::
Run() {
    while (true) {
        semaphore_.Wait();

        if (IsShutdown()) break;

        //dequeue job
        CutUpdateQueue::Job job = job_queue_.PopFrontJob();

        if (job.task_ != CutUpdateQueue::Task::CUT_INVALID_TASK) {
            switch (job.task_) {
                case CutUpdateQueue::Task::CUT_MASTER_TASK:
                    CutMaster();
                    break;

                case CutUpdateQueue::Task::CUT_ANALYSIS_TASK:
                    CutAnalysis(job.view_id_, job.model_id_);
                    break;

                case CutUpdateQueue::Task::CUT_UPDATE_TASK:
                    CutUpdate();
                    break;

                default: break;

            }
        }
    }

}

const bool CutUpdatePool::
Prepare() {
    CutDatabase* cut_database = CutDatabase::GetInstance();

    cut_database->ReceiveCameras(context_id_, user_cameras_);
    cut_database->ReceiveHeightDividedByTopMinusBottoms(context_id_, height_divided_by_top_minus_bottoms_);
    cut_database->ReceiveTransforms(context_id_, model_transforms_);
    cut_database->ReceiveThresholds(context_id_, model_thresholds_);

    transfer_list_.clear();
    render_list_.clear();

    gpu_cache_->ResetTransferList();
    gpu_cache_->set_transfer_budget(upload_budget_in_nodes_);
    gpu_cache_->set_transfer_slots_written(0);

    index_->UpdatePolicy(user_cameras_.size());

    //clamp threshold
    for (auto& threshold_it : model_thresholds_) {
        float& threshold = threshold_it.second;
        threshold = threshold < LAMURE_MIN_THRESHOLD ? LAMURE_MIN_THRESHOLD : threshold;
        threshold = threshold > LAMURE_MAX_THRESHOLD ? LAMURE_MAX_THRESHOLD : threshold;

    }

#ifdef LAMURE_CUT_UPDATE_ENABLE_MODEL_TIMEOUT
    ++cut_update_counter_;
    std::set<model_t> rendered_model_ids;
    cut_database->ReceiveRendered(context_id_, rendered_model_ids);

    for (const auto& model_id : rendered_model_ids) {
        model_freshness_[model_id] = cut_update_counter_;

    }
#endif


    //make sure roots are resident and aquired

    bool all_roots_resident = true;

    for (model_t model_id = 0; model_id < index_->num_models(); ++model_id) {
        for (view_t view_id = 0; view_id < index_->num_views(); ++view_id) {
            if (index_->GetCurrentCut(view_id, model_id).empty()) {
                all_roots_resident = false;
            }
        }
    }

    if (!all_roots_resident) {
        all_roots_resident = true;

        OocCache* ooc_cache = OocCache::GetInstance();

        ooc_cache->Lock();
        ooc_cache->Refresh();

        for (model_t model_id = 0; model_id < index_->num_models(); ++model_id) {
            if (!ooc_cache->IsNodeResident(model_id, 0)) {
                ooc_cache->RegisterNode(model_id, 0, 100);
                all_roots_resident = false;
            }
        }

        ooc_cache->Unlock();

        if (!all_roots_resident) {
            return false;
        }
        else {
            ooc_cache->Lock();
            gpu_cache_->Lock();
            for (model_t model_id = 0; model_id < index_->num_models(); ++model_id) {
                if (!gpu_cache_->IsNodeResident(model_id, 0)) {
                    gpu_cache_->RegisterNode(model_id, 0);
                }

                for (view_t view_id = 0; view_id < index_->num_views(); ++view_id) {
                    if (index_->GetCurrentCut(view_id, model_id).empty()) {
                        assert(ooc_cache->IsNodeResident(model_id, 0));
                        assert(gpu_cache_->IsNodeResident(model_id, 0));

                        ooc_cache->AquireNode(context_id_, view_id, model_id, 0);
                        gpu_cache_->AquireNode(context_id_, view_id, model_id, 0);

                        index_->PushAction(CutUpdateIndex::Action(CutUpdateIndex::Queue::KEEP, view_id, model_id, 0, 10000.f), false);

                    }
                }
            }

            gpu_cache_->Unlock();
            ooc_cache->Unlock();
        }
    }

    return true;

}

void CutUpdatePool::
CutMaster() {
    if (!Prepare()) {
        std::lock_guard<std::mutex> lock(mutex_);
        master_dispatched_ = false;
        return;
    }

#ifdef LAMURE_CUT_UPDATE_ENABLE_SHOW_GPU_CACHE_USAGE
    std::cout << "PLOD: free slots gpu : " << gpu_cache_->NumFreeSlots() << "\t\t( " << gpu_cache_->num_slots()-gpu_cache_->NumFreeSlots() << " occupied)" << std::endl;
#endif

#ifdef LAMURE_CUT_UPDATE_ENABLE_SHOW_OOC_CACHE_USAGE
    std::cout << "PLOD: free slots cpu: " << ooc_cache_->NumFreeSlots() << "\t\t( " << ooc_cache_->num_slots()-ooc_cache_->NumFreeSlots() << " occupied)" << std::endl << std::endl;
#endif

    //swap and use temporary buffer
    if (current_gpu_buffer_ == CutDatabaseRecord::TemporaryBuffer::BUFFER_A) {
        current_gpu_buffer_ = CutDatabaseRecord::TemporaryBuffer::BUFFER_B;
        current_gpu_storage_ = current_gpu_storage_B_;
    }
    else {
        current_gpu_buffer_ = CutDatabaseRecord::TemporaryBuffer::BUFFER_A;
        current_gpu_storage_ = current_gpu_storage_A_;
    }

#ifdef LAMURE_CUT_UPDATE_ENABLE_REPEAT_MODE

    uint32_t num_cut_updates = 0;
    AutoTimer tmr;

    while (true) {
        boost::timer::cpu_times const elapsed_times(tmr.elapsed());
        boost::timer::nanosecond_type const elapsed(elapsed_times.system + elapsed_times.user);

        {
            std::lock_guard<std::mutex> lock(mutex_);

            if ((num_cut_updates > 0 && elapsed >= last_frame_elapsed_*0.5f) || num_cut_updates >= LAMURE_CUT_UPDATE_MAX_NUM_UPDATES_PER_FRAME) {
                tmr.stop();
                break;
            }
        }

        ++num_cut_updates;

#endif


    //swap cut index
    index_->SwapCuts();

    assert(semaphore_.NumSignals() == 0);
    assert(master_semaphore_.NumSignals() == 0);

    //re-configure semaphores
    master_semaphore_.Lock();
    master_semaphore_.set_max_signal_count(index_->num_models() * index_->num_views());
    master_semaphore_.set_min_signal_count(index_->num_models() * index_->num_views());
    master_semaphore_.Unlock();

    semaphore_.Lock();
    semaphore_.set_max_signal_count(index_->num_models() * index_->num_views());
    semaphore_.set_min_signal_count(1);
    semaphore_.Unlock();

    //launch slaves
    for (view_t view_id = 0; view_id < index_->num_views(); ++view_id) {
        for (model_t model_id = 0; model_id < index_->num_models(); ++model_id) {
            job_queue_.PushJob(CutUpdateQueue::Job(CutUpdateQueue::Task::CUT_ANALYSIS_TASK, view_id, model_id));
        }
    }

    semaphore_.Signal(index_->num_models() * index_->num_views());

    master_semaphore_.Wait();
    if (IsShutdown())
        return;

    assert(semaphore_.NumSignals() == 0);
    assert(master_semaphore_.NumSignals() == 0);

    index_->Sort();

    //re-configure semaphores
    master_semaphore_.Lock();
    master_semaphore_.set_max_signal_count(1);
    master_semaphore_.set_min_signal_count(1);
    master_semaphore_.Unlock();

    semaphore_.Lock();
    semaphore_.set_max_signal_count(1);
    semaphore_.set_min_signal_count(1);
    semaphore_.Unlock();

    job_queue_.PushJob(CutUpdateQueue::Job(CutUpdateQueue::Task::CUT_UPDATE_TASK, 0, 0));
    semaphore_.Signal(1);

    master_semaphore_.Wait();
    if (IsShutdown())
        return;

#ifdef LAMURE_CUT_UPDATE_ENABLE_REPEAT_MODE
    }
#endif

    //apply changes
    {
        //ModelDatabase* database = ModelDatabase::GetInstance();
        CutDatabase* cuts = CutDatabase::GetInstance();

        cuts->LockRecord(context_id_);

        for (model_t model_id = 0; model_id < index_->num_models(); ++model_id) {
            for (view_t view_id = 0; view_id < index_->num_views(); ++view_id) {
                Cut cut(context_id_, view_id, model_id);
                cut.set_complete_set(render_list_[view_id][model_id]);

                cuts->SetCut(context_id_, view_id, model_id, cut);
            }
        }

        cuts->SetUpdatedSet(context_id_, transfer_list_);

        cuts->SetIsFrontModified(context_id_, gpu_cache_->transfer_budget() < upload_budget_in_nodes_); //...
        cuts->SetIsSwapRequired(context_id_, true);
        cuts->SetBuffer(context_id_, current_gpu_buffer_);

        cuts->UnlockRecord(context_id_);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            master_dispatched_ = false;
        }
    }


}


void CutUpdatePool::
CutAnalysis(view_t view_id, model_t model_id) {

    assert(view_id != invalid_view_t);
    assert(model_id != invalid_model_t);
    assert(view_id < index_->num_views());
    assert(model_id < index_->num_models());

    scm::math::mat4f model_matrix;
#ifdef LAMURE_CUT_UPDATE_ENABLE_MODEL_TIMEOUT
    size_t freshness;
#endif
    scm::gl::frustum frustum;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        model_matrix = model_transforms_[model_id];
#ifdef LAMURE_CUT_UPDATE_ENABLE_MODEL_TIMEOUT
        freshness = model_freshness_[model_id];
#endif
        frustum = user_cameras_[view_id].GetFrustumByModel(model_matrix);
    }

    //perform cut analysis
    std::set<node_t> old_cut = index_->GetPreviousCut(view_id, model_id);

    index_->ResetCut(view_id, model_id);

    uint32_t fan_factor = index_->FanFactor(model_id);

    bool freshness_timeout = false;
#ifdef LAMURE_CUT_UPDATE_ENABLE_MODEL_TIMEOUT
    freshness_timeout = cut_update_counter_ - freshness > LAMURE_CUT_UPDATE_MAX_MODEL_TIMEOUT;
#endif

    float min_error_threshold = model_thresholds_[model_id] - 0.1f;
    float max_error_threshold = model_thresholds_[model_id] + 0.1f;

    //cut analysis
    std::set<node_t>::iterator cut_it;
    for (cut_it = old_cut.begin(); cut_it != old_cut.end(); ++cut_it) {
        node_t node_id = *cut_it;

        bool all_siblings_in_cut = false;
        bool no_sibling_in_frustum = true;

        node_t parent_id = 0;
        float parent_error = 0;
        std::vector<node_t> siblings;

        assert(node_id != invalid_node_t);
        assert(node_id < index_->NumNodes(model_id));

        if (node_id > 0 && node_id < index_->NumNodes(model_id)) {
            parent_id = index_->GetParentId(model_id, node_id);
            parent_error = CalculateNodeError(view_id, model_id, parent_id);

            index_->GetAllSiblings(model_id, node_id, siblings);

            all_siblings_in_cut = IsAllNodesInCut(model_id, siblings, old_cut);
            no_sibling_in_frustum = !IsNodeInFrustum(view_id, model_id, parent_id, frustum);
        }

        if (!all_siblings_in_cut) {
            float node_error = CalculateNodeError(view_id, model_id, node_id);
            bool node_in_frustum = IsNodeInFrustum(view_id, model_id, node_id, frustum);

            if (node_in_frustum && node_error > max_error_threshold) {
                //only split if the predicted error of children does not require collapsing
                bool split = true;
                std::vector<node_t> children;
                index_->GetAllChildren(model_id, node_id, children);
                for (const auto& child_id : children) {
                    if (child_id == invalid_node_t)
                    {
                        split = false;
                        break;
                    }

                    float child_error = CalculateNodeError(view_id, model_id, child_id);
                    if (child_error < min_error_threshold)
                    {
                        split = false;
                        break;
                    }
                }
                if (!split || freshness_timeout) {
                    index_->PushAction(CutUpdateIndex::Action(CutUpdateIndex::Queue::KEEP, view_id, model_id, node_id, parent_error), false);
                }
                else {
                    index_->PushAction(CutUpdateIndex::Action(CutUpdateIndex::Queue::MUST_SPLIT,view_id, model_id, node_id, node_error), false);
                }
            }
            else {
                index_->PushAction(CutUpdateIndex::Action(CutUpdateIndex::Queue::KEEP, view_id, model_id, node_id, parent_error), false);
            }

        }
        else {
            if (no_sibling_in_frustum) {
#ifdef LAMURE_CUT_UPDATE_MUST_COLLAPSE_OUTSIDE_FRUSTUM
                index_->PushAction(CutUpdateIndex::Action(CutUpdateIndex::Queue::MUST_COLLAPSE, view_id, model_id, parent_id, parent_error), false);
#else
                index_->PushAction(CutUpdateIndex::Action(CutUpdateIndex::Queue::COLLAPSE_ON_NEED, view_id, model_id, parent_id, parent_error), false);
#endif
            }
            else {
                //the entire group of siblings is in the cut and visible

                if (freshness_timeout) {
                    index_->PushAction(CutUpdateIndex::Action(CutUpdateIndex::Queue::COLLAPSE_ON_NEED, view_id, model_id, parent_id, parent_error), false);

                    //skip to next group of siblings
                    std::advance(cut_it, fan_factor-1);
                    continue;
                }

                bool keep_all_siblings = true;

                bool all_sibling_errors_below_min_error_threshold = true;

                std::vector<bool> keep_sibling;

                for (const auto& sibling_id : siblings) {
                    float sibling_error = CalculateNodeError(view_id, model_id, sibling_id);
                    bool sibling_in_frustum = IsNodeInFrustum(view_id, model_id, sibling_id, frustum);

                    if (sibling_error > max_error_threshold && sibling_in_frustum) {
                        //only split if the predicted error of children does not require collapsing
                        bool split = true;
                        std::vector<node_t> children;
                        index_->GetAllChildren(model_id, sibling_id, children);
                        for (const auto& child_id : children) {
                            if (child_id == invalid_node_t) {
                                split = false;
                                break;
                            }

                            float child_error = CalculateNodeError(view_id, model_id, child_id);
                            if (child_error < min_error_threshold) {
                                split = false;
                                break;
                            }
                        }
                        if (!split) {
                            keep_sibling.push_back(true);
                        }
                        else {
                            index_->PushAction(CutUpdateIndex::Action(CutUpdateIndex::Queue::MUST_SPLIT, view_id, model_id, sibling_id, sibling_error), false);

                            keep_all_siblings = false;
                            keep_sibling.push_back(false);
                        }
                    }
                    else keep_sibling.push_back(true);

                    if (sibling_error >= min_error_threshold) {
                        all_sibling_errors_below_min_error_threshold = false;
                    }

                }


                if (keep_all_siblings && all_sibling_errors_below_min_error_threshold) {
                    index_->PushAction(CutUpdateIndex::Action(CutUpdateIndex::Queue::MUST_COLLAPSE, view_id, model_id, parent_id, parent_error), false);
                }
                else if (keep_all_siblings) {
                    index_->PushAction(CutUpdateIndex::Action(CutUpdateIndex::Queue::MAYBE_COLLAPSE, view_id, model_id, parent_id, parent_error), false);
                }
                else {
                    for (uint32_t j = 0; j < fan_factor; ++j) {
                        if (keep_sibling[j]) {
                            index_->PushAction(CutUpdateIndex::Action(CutUpdateIndex::Queue::KEEP, view_id, model_id, siblings[j], parent_error), false);
                        }
                    }
                }

            }

            //skip to next group of siblings
            std::advance(cut_it, fan_factor-1);
        }

    }

    master_semaphore_.Signal(1);


}

void CutUpdatePool::
CutUpdateSplitAgain(const CutUpdateIndex::Action& split_action) {

    std::vector<node_t> candidates;
    index_->GetAllChildren(split_action.model_id_, split_action.node_id_, candidates);

    scm::math::mat4f model_matrix = model_transforms_[split_action.model_id_];
    scm::gl::frustum frustum = user_cameras_[split_action.view_id_].GetFrustumByModel(model_matrix);

    float min_error_threshold = model_thresholds_[split_action.model_id_] - 0.1f;
    float max_error_threshold = model_thresholds_[split_action.model_id_] + 0.1f;
    
    for (const auto& candidate_id : candidates) {
        float node_error = CalculateNodeError(split_action.view_id_, split_action.model_id_, candidate_id);

        if (node_error > max_error_threshold) {
            //only split if the predicted error of children does not require collapsing
            bool split = true;
            std::vector<node_t> children;
            index_->GetAllChildren(split_action.model_id_, candidate_id, children);
            for (const auto& child_id : children) {
                if (child_id == invalid_node_t) {
                    split = false;
                    break;
                }

                float child_error = CalculateNodeError(split_action.view_id_, split_action.model_id_, child_id);
                if (child_error < min_error_threshold) {
                    split = false;
                    break;
                }
            }
            if (!split) {
                index_->PushAction(CutUpdateIndex::Action(CutUpdateIndex::Queue::KEEP, split_action.view_id_, split_action.model_id_, candidate_id, node_error), true);

            }
            else {
                index_->PushAction(CutUpdateIndex::Action(CutUpdateIndex::Queue::MUST_SPLIT, split_action.view_id_, split_action.model_id_, candidate_id, node_error), true);

            }
        }
        else {
            index_->PushAction(CutUpdateIndex::Action(CutUpdateIndex::Queue::KEEP, split_action.view_id_, split_action.model_id_, candidate_id, node_error), true);
        }
    }

}

void CutUpdatePool::
CutUpdate() {
    OocCache* ooc_cache = OocCache::GetInstance();
    ooc_cache->Lock();
    ooc_cache->Refresh();
    gpu_cache_->Lock();

    bool check_residency = true;

    bool all_children_in_ooc_cache = true;
    bool all_children_in_gpu_cache = true;

    //cut update
    while (index_->NumActions(CutUpdateIndex::Queue::MUST_SPLIT) > 0) {

        CutUpdateIndex::Action must_split_action = index_->FrontAction(CutUpdateIndex::Queue::MUST_SPLIT);
        size_t fan_factor = index_->FanFactor(must_split_action.model_id_);

#if 1


        if (check_residency) {


        std::vector<node_t> child_ids;
        index_->GetAllChildren(must_split_action.model_id_, must_split_action.node_id_, child_ids);

        for (const auto& child_id : child_ids) {
            if (!ooc_cache->IsNodeResident(must_split_action.model_id_, child_id)) {
                all_children_in_ooc_cache = false;
                if (!all_children_in_gpu_cache)
                    break;
            }
            if (!gpu_cache_->IsNodeResident(must_split_action.model_id_, child_id)) {
                all_children_in_gpu_cache = false;
                if (!all_children_in_ooc_cache)
                    break;
            }

        }

        if (all_children_in_ooc_cache && all_children_in_gpu_cache) {

            index_->PopFrontAction(CutUpdateIndex::Queue::MUST_SPLIT);

            for (const auto& child_id : child_ids) {
                gpu_cache_->AquireNode(context_id_, must_split_action.view_id_, must_split_action.model_id_, child_id);
                ooc_cache->AquireNode(context_id_, must_split_action.view_id_, must_split_action.model_id_, child_id);
            }

#ifdef LAMURE_CUT_UPDATE_ENABLE_SPLIT_AGAIN_MODE
            CutUpdateSplitAgain(must_split_action);
#else
            index_->ApproveAction(must_split_action);
#endif
            continue;

        }

        }

#endif
        check_residency = false;

        bool all_children_fit_in_ooc_cache = ooc_cache->NumFreeSlots() >= fan_factor;
        bool all_children_fit_in_gpu_cache = gpu_cache_->NumFreeSlots() >= fan_factor;

        if ((all_children_fit_in_ooc_cache && all_children_fit_in_gpu_cache)
            || (all_children_in_ooc_cache && all_children_fit_in_gpu_cache)) {
            CutUpdateIndex::Action msa = index_->FrontAction(CutUpdateIndex::Queue::MUST_SPLIT);
            index_->PopFrontAction(CutUpdateIndex::Queue::MUST_SPLIT);

            SplitNode(msa);
            check_residency = true;
            continue;
        }

        if (index_->NumActions(CutUpdateIndex::Queue::MUST_COLLAPSE) > 0) {
            CutUpdateIndex::Action collapse_action = index_->FrontAction(CutUpdateIndex::Queue::MUST_COLLAPSE);
            index_->PopFrontAction(CutUpdateIndex::Queue::MUST_COLLAPSE);

            CollapseNode(collapse_action);
            continue;
        }

        if (index_->NumActions(CutUpdateIndex::Queue::COLLAPSE_ON_NEED) > 0) {
            CutUpdateIndex::Action collapse_on_need_action = index_->FrontAction(CutUpdateIndex::Queue::COLLAPSE_ON_NEED);
            index_->PopFrontAction(CutUpdateIndex::Queue::COLLAPSE_ON_NEED);

            CollapseNode(collapse_on_need_action);
            continue;
        }

        if (index_->NumActions(CutUpdateIndex::Queue::MAYBE_COLLAPSE) > 0) {
            if (must_split_action.error_
                > index_->BackAction(CutUpdateIndex::Queue::MAYBE_COLLAPSE).error_) {
                CutUpdateIndex::Action collapse_action = index_->BackAction(CutUpdateIndex::Queue::MAYBE_COLLAPSE);
                index_->PopBackAction(CutUpdateIndex::Queue::MAYBE_COLLAPSE);

                CollapseNode(collapse_action);
                continue;
            }
        }

#ifdef LAMURE_CUT_UPDATE_ENABLE_CUT_UPDATE_EXPERIMENTAL_MODE
        if (index_->NumActions(CutUpdateIndex::Queue::KEEP) > 0) {
            CutUpdateIndex::Action keep_action = index_->BackAction(CutUpdateIndex::Queue::KEEP);
            index_->PopBackAction(CutUpdateIndex::Queue::KEEP);

            if (must_split_action.error_ > keep_action.error_) {
                node_t keep_action_parent_id = index_->GetParentId(keep_action.model_id_, keep_action.node_id_);

                if (keep_action.node_id_ > 0 && keep_action_parent_id > 0) {

                    std::vector<node_t> siblings;
                    index_->GetAllSiblings(keep_action.model_id_, keep_action.node_id_, siblings);

                    if (IsAllNodesInCut(keep_action.model_id_, siblings, index_->GetPreviousCut(keep_action.view_id_, keep_action.model_id_))) {
                        bool singularity = false;

                        for (const auto& sibling_id : siblings) {
                            if (singularity)
                                break;

                            if (sibling_id == must_split_action.node_id_) {
                                singularity = true;
                                break;
                            }

                            std::vector<node_t> sibling_children;
                            index_->GetAllChildren(keep_action.model_id_, sibling_id, sibling_children);

                            for (const auto& sibling_child_id : sibling_children) {
                                if (sibling_child_id == must_split_action.node_id_) {
                                    singularity = true;
                                    break;
                                }
                            }

                        }

                        if (!singularity) {
                            for (const auto& sibling_id : siblings) {
                                if (sibling_id != invalid_node_t) {

                                    //cancel all possible actions on sibling_id
                                    index_->CancelAction(keep_action.view_id_, keep_action.model_id_, sibling_id);

                                    if (gpu_cache_->ReleaseNodeInvalidate(context_id_,
                                                                          keep_action.view_id_,
                                                                          keep_action.model_id_,
                                                                          sibling_id)) {
                                        gpu_cache_->RemoveFromTransferList(keep_action.model_id_, sibling_id);
                                    }

                                    ooc_cache->ReleaseNode(context_id_, keep_action.view_id_, keep_action.model_id_, sibling_id);

                                    //cancel a possible split action that already happened
                                    std::vector<node_t> sibling_children;
                                    index_->GetAllChildren(keep_action.model_id_, sibling_id, sibling_children);
                                    for (const auto& sibling_child_id : sibling_children) {
                                        if (sibling_child_id != invalid_node_t) {
                                            index_->CancelAction(keep_action.view_id_, keep_action.model_id_, sibling_child_id);

                                            if (gpu_cache_->ReleaseNodeInvalidate(context_id_,
                                                                                keep_action.view_id_,
                                                                                keep_action.model_id_,
                                                                                sibling_child_id)) {
                                                gpu_cache_->RemoveFromTransferList(keep_action.model_id_, sibling_child_id);
                                            }

                                            ooc_cache->ReleaseNode(context_id_, keep_action.view_id_, keep_action.model_id_, sibling_child_id);
                                        }
                                    }
                                }
                            }

                            assert(gpu_cache_->IsNodeResident(keep_action.model_id_, keep_action_parent_id));
                            assert(ooc_cache->IsNodeResident(keep_action.model_id_, keep_action_parent_id));

                            index_->ApproveAction(
                                CutUpdateIndex::Action(CutUpdateIndex::Queue::KEEP,
                                                       keep_action.view_id_,
                                                       keep_action.model_id_,
                                                       keep_action_parent_id,
                                                       keep_action.error_));

                            continue;
                        }
                    }
                }
            }

            //approve keep action
            index_->ApproveAction(keep_action);

        }


        if (index_->NumActions(CutUpdateIndex::Queue::MUST_SPLIT) > 1) { //> 1, prevent request from canceling itself
            CutUpdateIndex::Action split_action = index_->BackAction(CutUpdateIndex::Queue::MUST_SPLIT);
            index_->PopBackAction(CutUpdateIndex::Queue::MUST_SPLIT);

            if (must_split_action.error_ > split_action.error_) {
                node_t split_action_parent_id = index_->GetParentId(split_action.model_id_, split_action.node_id_);

                if (split_action.node_id_ > 0 && split_action_parent_id > 0) {

                    //only if siblings are also in cut -- why does it make sense to check this here?
                    //because we check it above in the keep-action branch anyway
                    //and there is really no reason to cancel this action if we cannot use it
                    //to cut down memory usage in the end.
                    std::vector<node_t> siblings;
                    index_->GetAllSiblings(split_action.model_id_, split_action.node_id_, siblings);

                    if (IsAllNodesInCut(split_action.model_id_,
                                        siblings,
                                        index_->GetPreviousCut(split_action.view_id_, split_action.model_id_))) {

                        bool singularity = split_action.node_id_ == must_split_action.node_id_;

                        std::vector<node_t> split_children;
                        index_->GetAllChildren(split_action.model_id_, split_action.node_id_, split_children);

                        //check if children of split_action are equal to the must_split_node (parent of must_split)
                        //this is important, since it would not free any memory anyways to cancel a split_action that is above
                        //the must_split action in the hierarchy
                        for (const auto& split_child_id : split_children) {
                            if (split_child_id == must_split_action.node_id_) {
                                singularity = true;
                                break;
                            }
                        }

                        if (!singularity) {
                            assert(gpu_cache_->IsNodeResident(split_action.model_id_, split_action.node_id_));
                            assert(ooc_cache->IsNodeResident(split_action.model_id_, split_action.node_id_));

                            scm::math::mat4f model_matrix = model_transforms_[split_action.model_id_];

                            float replacement_node_error = CalculateNodeError(split_action.view_id_,
                                                                              split_action.model_id_,
                                                                              split_action.node_id_);
                            index_->PushAction(
                                CutUpdateIndex::Action(CutUpdateIndex::Queue::KEEP,
                                                       split_action.view_id_,
                                                       split_action.model_id_,
                                                       split_action.node_id_,
                                                       replacement_node_error*2.75f),
                                true);

                            continue;
                        }
                    }

                }

            }

            //reject split action
            index_->RejectAction(split_action);

        }

#endif

        //no success, reject must split action
        CutUpdateIndex::Action msa = index_->FrontAction(CutUpdateIndex::Queue::MUST_SPLIT);
        index_->PopFrontAction(CutUpdateIndex::Queue::MUST_SPLIT);
        index_->RejectAction(msa);
        check_residency = true;

    }

    //approve all remaining must-collapse-actions
    while (index_->NumActions(CutUpdateIndex::Queue::MUST_COLLAPSE) > 0) {
        CutUpdateIndex::Action collapse_action = index_->FrontAction(CutUpdateIndex::Queue::MUST_COLLAPSE);
        index_->PopFrontAction(CutUpdateIndex::Queue::MUST_COLLAPSE);
        CollapseNode(collapse_action);

    }

#ifdef LAMURE_CUT_UPDATE_ENABLE_PREFETCHING
    PrefetchRoutine();      
#endif
    gpu_cache_->Unlock();
    ooc_cache->Unlock();

    //reject remaining collapse-on-need-actions
    while (index_->NumActions(CutUpdateIndex::Queue::COLLAPSE_ON_NEED) > 0) {
        CutUpdateIndex::Action collapse_on_need_action = index_->FrontAction(CutUpdateIndex::Queue::COLLAPSE_ON_NEED);
        index_->PopFrontAction(CutUpdateIndex::Queue::COLLAPSE_ON_NEED);
        index_->RejectAction(collapse_on_need_action);

    }

    //reject remaining maybe-collapse-actions
    while (index_->NumActions(CutUpdateIndex::Queue::MAYBE_COLLAPSE) > 0) {
        CutUpdateIndex::Action maybe_collapse_action = index_->FrontAction(CutUpdateIndex::Queue::MAYBE_COLLAPSE);
        index_->PopFrontAction(CutUpdateIndex::Queue::MAYBE_COLLAPSE);
        index_->RejectAction(maybe_collapse_action);

    }

    //approve all keep-actions
    while (index_->NumActions(CutUpdateIndex::Queue::KEEP) > 0) {
        CutUpdateIndex::Action keep_action = index_->FrontAction(CutUpdateIndex::Queue::KEEP);
        index_->PopFrontAction(CutUpdateIndex::Queue::KEEP);
        index_->ApproveAction(keep_action);

    }

    assert(index_->NumActions(CutUpdateIndex::Queue::KEEP) == 0);
    assert(index_->NumActions(CutUpdateIndex::Queue::MUST_SPLIT) == 0);
    assert(index_->NumActions(CutUpdateIndex::Queue::MUST_COLLAPSE) == 0);
    assert(index_->NumActions(CutUpdateIndex::Queue::COLLAPSE_ON_NEED) == 0);
    assert(index_->NumActions(CutUpdateIndex::Queue::MAYBE_COLLAPSE) == 0);

    CompileRenderList();
    CompileTransferList();

    master_semaphore_.Signal(1);

}

void CutUpdatePool::
CompileRenderList() {
    render_list_.clear();

    const std::set<view_t>& view_ids = index_->view_ids();

    for (const auto  view_id : view_ids) {
        std::vector<std::vector<Cut::NodeSlotAggregate>> view_render_lists;

        for (model_t model_id = 0; model_id < index_->num_models(); ++model_id) {
            std::vector<Cut::NodeSlotAggregate> model_render_lists;

            const std::set<node_t>& current_cut = index_->GetCurrentCut(view_id, model_id);

            for (const auto& node_id : current_cut) {
                model_render_lists.push_back(Cut::NodeSlotAggregate(node_id, gpu_cache_->SlotId(model_id, node_id)));
            }

            view_render_lists.push_back(model_render_lists);
        }
        render_list_.push_back(view_render_lists);
    }

}


#ifdef LAMURE_CUT_UPDATE_ENABLE_PREFETCHING
void CutUpdatePool::
PrefetchRoutine() {

   OocCache* ooc_cache = OocCache::GetInstance();

#if 0
   uint32_t num_prefetched = 0;
#endif

   std::queue<std::pair<model_t, node_t>> node_id_queue;
   for (const auto& action : pending_prefetch_set_) {
      if (action.node_id_ == invalid_node_t) {
          continue;
      }
      
      float max_error_threshold = model_thresholds_[model_id] + 0.1f;

      if (action.error_ > max_error_threshold * LAMURE_CUT_UPDATE_PREFETCH_FACTOR) {
 
         std::vector<node_t> child_ids;
         index_->GetAllChildren(action.model_id_, action.node_id_, child_ids);
         for (const auto& child_id : child_ids) {
            node_id_queue.push(std::make_pair(action.model_id_, child_id));
         }
      }
   }


   uint32_t current_prefetch_depth = 0; 
   
   while (!node_id_queue.empty()) {
 
     std::pair<model_t, node_t> model_node = node_id_queue.front();
     node_t node_id = model_node.second;
     model_t model_id = model_node.first;
     node_id_queue.pop();

     std::vector<node_t> child_ids;
     index_->GetAllChildren(model_id, node_id, child_ids);
            
     uint32_t fan_factor = index_->FanFactor(model_id);
     if (++current_prefetch_depth < LAMURE_CUT_UPDATE_PREFETCH_BUDGET) {
        if (ooc_cache->NumFreeSlots() > ooc_cache->num_slots()/4
           && gpu_cache_->NumFreeSlots() > gpu_cache_->num_slots()/4) {
 
           bool all_children_fit_in_ooc_cache = ooc_cache->NumFreeSlots() >= fan_factor;
           bool all_children_fit_in_gpu_cache = gpu_cache_->NumFreeSlots() >= fan_factor;     

           if (all_children_fit_in_ooc_cache && all_children_fit_in_gpu_cache) {
              for (const auto& child_id : child_ids) {

                if (child_id == invalid_node_t) 
                   continue;

                if (!ooc_cache->IsNodeResident(model_id, child_id)) {
                   //load child from harddisk
                   if (ooc_cache->NumFreeSlots() > 0) {
                      ooc_cache->RegisterNode(model_id, child_id, (int32_t)(-current_prefetch_depth));
                   }
                }
                    
                node_id_queue.push(std::make_pair(model_id, child_id));
#if 0
                ++num_prefetched;
#endif
              }
           } 
           else {
              break;
           }
             
           
        }
     }
  }

   pending_prefetch_set_.clear();

#if 0
   std::cout << "PLOD: num prefetched: " << num_prefetched << std::endl;
#endif


}
#endif

void CutUpdatePool::
CompileTransferList() {
    ModelDatabase* database = ModelDatabase::GetInstance();
    OocCache* ooc_cache = OocCache::GetInstance();

    const std::vector<std::unordered_set<node_t>>& transfer_list = gpu_cache_->transfer_list();

    slot_t slot_count = gpu_cache_->transfer_slots_written();
    size_t stride_in_bytes = database->size_of_surfel() * database->surfels_per_node();

    for (model_t model_id = 0; model_id < index_->num_models(); ++model_id) {
        for (const auto& node_id : transfer_list[model_id]) {
            slot_t slot_id = gpu_cache_->SlotId(model_id, node_id);

            assert(slot_id < (slot_t)render_budget_in_nodes_);

            char* node_data = ooc_cache->NodeData(model_id, node_id);

            memcpy(current_gpu_storage_ + slot_count*stride_in_bytes, node_data, stride_in_bytes);

            transfer_list_.push_back(CutDatabaseRecord::SlotUpdateDescr(slot_count, slot_id));

            ++slot_count;

        }

    }

    gpu_cache_->ResetTransferList();
    gpu_cache_->set_transfer_slots_written(slot_count);


}

void CutUpdatePool::
SplitNode(const CutUpdateIndex::Action& action) {

    std::vector<node_t> child_ids;
    index_->GetAllChildren(action.model_id_, action.node_id_, child_ids);

    //return if children are invalid node ids
    if (child_ids[0] == invalid_node_t || action.node_id_ == invalid_node_t) {
        index_->RejectAction(action);
        return;
    }

    size_t fan_factor = index_->FanFactor(action.model_id_);

    assert(child_ids[0] < index_->NumNodes(action.model_id_));

    bool all_children_available = true;

    OocCache* ooc_cache = OocCache::GetInstance();
    bool all_children_fit_in_ooc_cache = ooc_cache->NumFreeSlots() >= fan_factor;
    bool all_children_fit_in_gpu_cache = gpu_cache_->transfer_budget() >= fan_factor && gpu_cache_->NumFreeSlots() >= fan_factor;

    //try to obtain children
    for (const auto& child_id : child_ids) {
        if (!ooc_cache->IsNodeResident(action.model_id_, child_id)) {
            if (all_children_fit_in_ooc_cache) {
                //load child from harddisk
                if (ooc_cache->NumFreeSlots() > 0) {
                    ooc_cache->RegisterNode(action.model_id_, child_id, (int32_t)action.error_);
                }
            }
            all_children_available = false;
        }
    }

    if (all_children_available) {
        for (const auto& child_id : child_ids) {
            if (!gpu_cache_->IsNodeResident(action.model_id_, child_id)) {
                if (all_children_fit_in_gpu_cache) {
                    //transfer child to gpu
                    if (gpu_cache_->transfer_budget() > 0 && gpu_cache_->NumFreeSlots() > 0) {
                        if (gpu_cache_->RegisterNode(action.model_id_, child_id)) {
#ifdef LAMURE_CUT_UPDATE_ENABLE_PREFETCHING
                           pending_prefetch_set_.push_back(action);
#endif
                        }

                    }
                    else {
                        all_children_available = false;
                    }
                }
                else {
                    all_children_available = false;
                }
            }
        }
    }

    if (all_children_available) {
        for (const auto& child_id : child_ids) {
            gpu_cache_->AquireNode(context_id_, action.view_id_, action.model_id_, child_id);
            ooc_cache->AquireNode(context_id_, action.view_id_, action.model_id_, child_id);
        }

#ifdef LAMURE_CUT_UPDATE_ENABLE_SPLIT_AGAIN_MODE
        CutUpdateSplitAgain(action);
#else
        index_->ApproveAction(action);
#endif
    }
    else {
        index_->RejectAction(action);
    }

}

void CutUpdatePool::
CollapseNode(const CutUpdateIndex::Action& action) {

    //return if parent is invalid node id
    if (action.node_id_ < 1 || action.node_id_ == invalid_node_t) {
        index_->RejectAction(action);
        return;
    }

    std::vector<node_t> child_ids;
    index_->GetAllChildren(action.model_id_, action.node_id_, child_ids);

    OocCache* ooc_cache = OocCache::GetInstance();

    for (const auto& child_id : child_ids) {
        gpu_cache_->ReleaseNode(context_id_, action.view_id_, action.model_id_, child_id);
        ooc_cache->ReleaseNode(context_id_, action.view_id_, action.model_id_, child_id);
    }

    index_->ApproveAction(action);

}

const bool CutUpdatePool::
IsAllNodesInCut(const model_t model_id, const std::vector<node_t>& node_ids, const std::set<node_t>& cut) {
    for (node_t i = 0; i < node_ids.size(); ++i) {
        node_t node_id = node_ids[i];

        if (node_id >= (node_t)index_->NumNodes(model_id))
            return false;

        if (node_id == invalid_node_t)
            return false;

        if (cut.find(node_id) == cut.end())
            return false;
    }

    return true;
}

const bool CutUpdatePool::
IsNodeInFrustum(const view_t view_id, const model_t model_id, const node_t node_id, const scm::gl::frustum& frustum) {
    ModelDatabase* database = ModelDatabase::GetInstance();
    return 1 != user_cameras_[view_id].CullAgainstFrustum(frustum, database->GetModel(model_id)->bvh()->bounding_boxes()[node_id]);
}

const bool CutUpdatePool::
IsNoNodeInFrustum(const view_t view_id, const model_t model_id, const std::vector<node_t>& node_ids, const scm::gl::frustum& frustum) {
    for (const auto& node_id : node_ids) {
        if (node_id >= (node_t)index_->NumNodes(model_id))
            return false;

        if (node_id == invalid_node_t)
            return false;

        ModelDatabase* database = ModelDatabase::GetInstance();
        if (1 != user_cameras_[view_id].CullAgainstFrustum(frustum, database->GetModel(model_id)->bvh()->bounding_boxes()[node_id]))
            return false;
    }

    return true;
}


const float CutUpdatePool::
CalculateNodeError(const view_t view_id, const model_t model_id, const node_t node_id) {

    ModelDatabase* database = ModelDatabase::GetInstance();
    auto bvh = database->GetModel(model_id)->bvh();

    const scm::math::mat4f& model_matrix = model_transforms_[model_id];
    const scm::math::mat4f& view_matrix = user_cameras_[view_id].GetViewMatrix();

    float radius_scaling = scm::math::length(model_matrix * scm::math::vec4f(1.0f,0.f,0.f,0.f));
    float representative_radius = bvh->GetAvgSurfelRadius(node_id) * radius_scaling;

    auto bb = bvh->bounding_boxes()[node_id];


#if 1
 
    //original error computation
    scm::math::vec3f view_position = view_matrix * model_matrix * bvh->centroids()[node_id];
    float near_plane = user_cameras_[view_id].near_plane_value();
    float height_divided_by_top_minus_bottom = height_divided_by_top_minus_bottoms_[view_id];
    float error = std::abs(2.0f * representative_radius * (near_plane/-view_position.z) * height_divided_by_top_minus_bottom);

#else
    
    const scm::math::mat4f& proj_matrix = user_cameras_[view_id].GetProjectionMatrix();

    scm::math::mat4 cm = scm::math::inverse(view_matrix);
    scm::math::vec3f position = model_matrix * bvh->centroids()[node_id];
    scm::math::vec3 u = scm::math::vec3(cm[0], cm[1], cm[2]);
    scm::math::vec3 perimeter = position + (scm::math::normalize(u) * representative_radius);
    
    //project the position and the perimeter point, take their distance
    scm::math::mat4 view_projection = proj_matrix * view_matrix;
    scm::math::vec4 view_position = view_projection * position;
    scm::math::vec4 view_perimeter = view_projection * perimeter;
    float error = scm::math::length(view_position/view_position.w - view_perimeter/view_perimeter.w);
    //std::cout << "PLOD: " << view_position << "  ----  " << view_perimeter << " ,,,, " << error << std::endl;
#endif

    return error;
}



} // namespace ren

} // namespace lamure


