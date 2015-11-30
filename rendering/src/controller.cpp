// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/controller.h>

namespace lamure
{

namespace ren
{

std::mutex Controller::mutex_;
bool Controller::is_instanced_ = false;
Controller* Controller::single_ = nullptr;

Controller::
Controller()
: num_contexts_registered_(0),
  num_models_registered_(0) {

}

Controller::
~Controller() {
    std::lock_guard<std::mutex> lock(mutex_);

    is_instanced_ = false;

    for (auto& cut_update_pool_it : cut_update_pools_) {
        CutUpdatePool* pool = cut_update_pool_it.second;
        if (pool != nullptr) {
            delete pool;
            pool = nullptr;
        }
    }
    cut_update_pools_.clear();

    for (auto& gpu_context_it : gpu_contexts_) {
        GpuContext* ctx = gpu_context_it.second;
        if (ctx != nullptr) {
            delete ctx;
            ctx = nullptr;
        }
    }
    gpu_contexts_.clear();


}

Controller* Controller::
get_instance() {
    if (!is_instanced_) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!is_instanced_) {
            single_ = new Controller();
            is_instanced_ = true;
        }

        return single_;
    }
    else {
        return single_;
    }
}


const context_t Controller::
NumContextsRegistered() {
    std::lock_guard<std::mutex> lock(mutex_);

    return num_contexts_registered_;
}


void Controller::
SignalSystemreset() {
    std::lock_guard<std::mutex> lock(mutex_);

    Policy* policy = Policy::get_instance();
    policy->set_reset_system(true);

    for (const auto& context : context_map_) {
        context_t context_id = context.second;
        reset_flags_history_[context_id].push(true);
    }
}

const bool Controller::
IsSystemresetSignaled(const context_t context_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!reset_flags_history_[context_id].empty()) {
        bool reset = reset_flags_history_[context_id].front();
        reset_flags_history_[context_id].pop();
        if (reset) {
            return true;
        }
    }

    return false;

}

void Controller::
resetSystem() {
    Policy* policy = Policy::get_instance();

    if (policy->reset_system()) {
        Modeldatabase* database = Modeldatabase::get_instance();
        Cutdatabase* cuts = Cutdatabase::get_instance();
        Controller* controller = Controller::get_instance();

        context_t num_contexts_registered = controller->NumContextsRegistered();
        for (context_t ctx_id = 0; ctx_id < num_contexts_registered; ++ctx_id) {
            while (controller->IsCutUpdateInProgress(ctx_id)) {};
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if (policy->reset_system()) {
            database->Apply();
            cuts->reset();

            for (auto& cut_update_pool_it : cut_update_pools_) {
                CutUpdatePool* pool = cut_update_pool_it.second;
                if (pool != nullptr) {
                    while (pool->IsRunning()) {};
                    delete pool;
                    pool = nullptr;
                }
            }

            cut_update_pools_.clear();

            for (auto& gpu_context_it : gpu_contexts_) {
                GpuContext* context = gpu_context_it.second;
                if (context != nullptr) {
                    delete context;
                    context = nullptr;
                }

            }

            gpu_contexts_.clear();

            //disregard:
            //num_contexts_registered_ = 0;
            //num_views_registered_.clear();
            //context_map_.clear();

            //keep the model map!

            policy->set_reset_system(false);

        }
    }
}

context_t Controller::
DeduceContextId(const gua_context_desc_t context_desc) {
    auto context_it = context_map_.find(context_desc);

    if (context_it != context_map_.end()) {
        return context_map_[context_desc];
    }
    else {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            context_map_[context_desc] = num_contexts_registered_;
            view_map_[num_contexts_registered_] = std::unordered_map<context_t, view_t>();
            num_views_registered_.push_back(0);

            gpu_contexts_.insert(std::make_pair(num_contexts_registered_, new GpuContext(num_contexts_registered_)));
#ifdef LAMURE_ENABLE_INFO
            std::cout << "PLOD: registered context id " << num_contexts_registered_ << std::endl;
#endif
            ++num_contexts_registered_;
        }
        return DeduceContextId(context_desc);
    }
}

view_t Controller::
DeduceViewId(const gua_context_desc_t context_desc, const gua_view_desc_t view_desc) {
    context_t context_id = DeduceContextId(context_desc);

    auto view_it = view_map_[context_id].find(view_desc);

    if (view_it != view_map_[context_id].end()) {
        return view_map_[context_id][view_desc];
    }
    else {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            view_map_[context_id][view_desc] = num_views_registered_[context_id];
#ifdef LAMURE_ENABLE_INFO
            std::cout << "PLOD: registered view id " << num_views_registered_[context_id] << " on context id " << context_id << std::endl;
#endif
            ++num_views_registered_[context_id];
        }
        return DeduceViewId(context_desc, view_desc);
    }


}

model_t Controller::
DeduceModelId(const gua_model_desc_t& model_desc) {
    auto model_it = model_map_.find(model_desc);

    if (model_it != model_map_.end()) {
        return model_map_[model_desc];
    }
    else {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            model_map_[model_desc] = num_models_registered_;
#ifdef LAMURE_ENABLE_INFO
            std::cout << "PLOD: registered model id " << num_models_registered_ << std::endl;
#endif
            ++num_models_registered_;
        }

        return DeduceModelId(model_desc);
    }

}

const bool Controller::
IsCutUpdateInProgress(const context_t context_id) {
    auto gpu_context_it = gpu_contexts_.find(context_id);

    if (gpu_context_it == gpu_contexts_.end()) {
        throw std::runtime_error(
            "PLOD: Controller::Gpu Context not found for context: " + context_id);
    }

    auto cut_update_it = cut_update_pools_.find(context_id);

    if (cut_update_it != cut_update_pools_.end()) {
        return cut_update_it->second->IsRunning();
    }
    else {
        GpuContext* ctx = gpu_context_it->second;
        if (!ctx->is_created()) {
            throw std::runtime_error(
                "PLOD: Controller::Gpu Context not created for context: " + context_id);
        }
        cut_update_pools_[context_id] = new CutUpdatePool(context_id, ctx->upload_budget_in_nodes(), ctx->render_budget_in_nodes());
        return IsCutUpdateInProgress(context_id);
    }

    return true;
}

void Controller::
Dispatch(const context_t context_id, scm::gl::render_device_ptr device) {
    auto gpu_context_it = gpu_contexts_.find(context_id);

    if (gpu_context_it == gpu_contexts_.end()) {
        throw std::runtime_error(
            "PLOD: Controller::Gpu Context not found for context: " + context_id);
    }

    auto cut_update_it = cut_update_pools_.find(context_id);

    if (cut_update_it != cut_update_pools_.end()) {

        lamure::ren::Cutdatabase* cuts = lamure::ren::Cutdatabase::get_instance();
        cuts->Swap(context_id);

        cut_update_it->second->DispatchCutUpdate(
            gpu_context_it->second->temporary_storages().storage_a_,
            gpu_context_it->second->temporary_storages().storage_b_);

        if (cuts->IsfrontModified(context_id)) {
            CutdatabaseRecord::Temporarybuffer current = cuts->Getbuffer(context_id);

            GpuContext* ctx = gpu_context_it->second;
            ctx->UnmapTempStorage(current, device);
            ctx->UpdatePrimarybuffer(current, device);
            cuts->SignalUploadcomplete(context_id);
            ctx->MapTempStorage(current, device);

        }
    }
    else {
        GpuContext* ctx = gpu_context_it->second;
        if (!ctx->is_created()) {
            //throw std::runtime_error(
            //    "PLOD: Controller::Gpu Context not created for context: " + context_id);

            //fix for gua:
            ctx->Create(device);
        }

        cut_update_pools_[context_id] = new CutUpdatePool(context_id, ctx->upload_budget_in_nodes(), ctx->render_budget_in_nodes());
        Dispatch(context_id, device);
    }

}

const bool Controller::
IsModelPresent(const gua_model_desc_t model_desc) {
    return model_map_.find(model_desc) != model_map_.end();

}


scm::gl::buffer_ptr Controller::
GetContextbuffer(const context_t context_id, scm::gl::render_device_ptr device) {
    auto gpu_context_it = gpu_contexts_.find(context_id);

    if (gpu_context_it == gpu_contexts_.end()) {
        throw std::runtime_error(
            "PLOD: Controller::Gpu Context not found for context: " + context_id);
    }

    return gpu_context_it->second->GetContextbuffer(device);

}


scm::gl::vertex_array_ptr Controller::
GetContextMemory(const context_t context_id, scm::gl::render_device_ptr device) {
    auto gpu_context_it = gpu_contexts_.find(context_id);

    if (gpu_context_it == gpu_contexts_.end()) {
        throw std::runtime_error(
            "PLOD: Controller::Gpu Context not found for context: " + context_id);
    }

    return gpu_context_it->second->GetContextMemory(device);

}



} // namespace ren

} // namespace lamure


