// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/ooc_pool.h>

namespace lamure
{

namespace ren
{

OocPool::
OocPool(const uint32_t num_threads, const size_t size_of_slot_in_bytes)
: locked_(false),
  size_of_slot_(size_of_slot_in_bytes),
  num_threads_(num_threads),
  shutdown_(false),
  bytes_loaded_(0) {
    assert(num_threads_ > 0);

    //configure semaphore
    semaphore_.set_min_signal_count(1);
    semaphore_.set_max_signal_count(std::numeric_limits<size_t>::max());

    ModelDatabase* database = ModelDatabase::GetInstance();

    priority_queue_.Initialize(LAMURE_CUT_UPDATE_LOADING_QUEUE_MODE, database->num_models());

    for (uint32_t i = 0; i < num_threads_; ++i) {
        threads_.push_back(std::thread(&OocPool::Run, this));
    }
}

OocPool::
~OocPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        shutdown_ = true;
        semaphore_.Shutdown();
    }

    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads_.clear();

}

bool OocPool::
IsShutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    return shutdown_;
}

void OocPool::
Lock() {
    mutex_.lock();
    locked_ = true;
}

void OocPool::
Unlock() {
    locked_ = false;
    mutex_.unlock();
}

void OocPool::
StartMeasure() {
  std::lock_guard<std::mutex> lock(mutex_);
  bytes_loaded_ = 0;
}

void OocPool::
EndMeasure() {
  std::lock_guard<std::mutex> lock(mutex_);
  std::cout << "megabytes loaded: " << bytes_loaded_ / 1024 / 1024 << std::endl;
}

void OocPool::
Run() {
    ModelDatabase* database = ModelDatabase::GetInstance();
    model_t num_models = database->num_models();

    std::vector<LodStream*> lod_streams;

    for (model_t model_id = 0; model_id < num_models; ++model_id) {
        std::string bvh_filename = database->GetModel(model_id)->bvh()->filename();
        std::string lod_file_name = bvh_filename.substr(0, bvh_filename.size()-3) + "lod";

        LodStream* access = new LodStream();
        access->Open(lod_file_name);
        lod_streams.push_back(access);
    }

    char* local_cache = new char[size_of_slot_];

    while (true) {
        semaphore_.Wait();

        if (IsShutdown())
            break;

        CacheQueue::Job job = priority_queue_.TopJob();

        if (job.node_id_ != invalid_node_t) {
            assert(job.slot_mem_ != nullptr);

            size_t stride_in_bytes = database->size_of_surfel() * 
               database->GetModel(job.model_id_)->bvh()->surfels_per_node();
            size_t offset_in_bytes = job.node_id_ * stride_in_bytes;

            lod_streams[job.model_id_]->Read(local_cache, offset_in_bytes, stride_in_bytes);

            {
                std::lock_guard<std::mutex> lock(mutex_);
                bytes_loaded_ += stride_in_bytes;
                memcpy(job.slot_mem_, local_cache, stride_in_bytes);
                history_.push_back(job);
            }

        }

    }

    {
        for (model_t model_id = 0; model_id < num_models; ++model_id) {
            LodStream* lod_stream = lod_streams[model_id];
            if (lod_stream != nullptr) {
                lod_stream->Close();
                delete lod_stream;
                lod_stream = nullptr;
            }
        }
        lod_streams.clear();

        if (local_cache != nullptr) {
            delete[] local_cache;
            local_cache = nullptr;
        }
    }
}

void OocPool::
ResolveCacheHistory(CacheIndex* index) {
    assert(locked_);

    for (auto entry : history_) {
        index->ApplySlot(entry.slot_id_, entry.model_id_, entry.node_id_);
        priority_queue_.PopJob(entry);
    }

    history_.clear();

}

void OocPool::
PerformQueueMaintenance(CacheIndex* index) {
    assert(locked_);

    size_t num_jobs = priority_queue_.NumJobs();
    for (size_t i = 0; i < num_jobs; ++i) {
        CacheQueue::Job job = priority_queue_.TopJob();

        assert(job.slot_id_ != invalid_slot_t);

        priority_queue_.PopJob(job);
        index->UnreserveSlot(job.slot_id_);

    }
}

bool OocPool::
AcknowledgeRequest(CacheQueue::Job job) {
    bool success = priority_queue_.PushJob(job);

    if (success) {
        semaphore_.Signal(1);
    }

    return success;
}

CacheQueue::QueryResult OocPool::
AcknowledgeQuery(const model_t model_id, const node_t node_id) {
    return priority_queue_.IsNodeIndexed(model_id, node_id);
}

void OocPool::
AcknowledgeUpdate(const model_t model_id, const node_t node_id, int32_t priority) {
    priority_queue_.UpdateJob(model_id, node_id, priority);
}



} // namespace ren

} // namespace lamure


