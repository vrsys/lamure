// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/ooc_cache.h>


namespace lamure
{

namespace ren
{

std::mutex OocCache::mutex_;
bool OocCache::is_instanced_ = false;
OocCache* OocCache::single_ = nullptr;

OocCache::
OocCache(const slot_t num_slots)
: Cache(num_slots),
  maintenance_counter_(0) {
    ModelDatabase* database = ModelDatabase::GetInstance();

    cache_data_ = new char[num_slots * database->size_of_surfel() * database->surfels_per_node()];

    pool_ = new OocPool(LAMURE_CUT_UPDATE_NUM_LOADING_THREADS,
                        database->surfels_per_node() * database->size_of_surfel());

#ifdef LAMURE_ENABLE_INFO
    std::cout << "PLOD: ooc-cache init" << std::endl;
#endif

}

OocCache::
~OocCache() {
    std::lock_guard<std::mutex> lock(mutex_);

    is_instanced_ = false;

    if (pool_ != nullptr) {
        delete pool_;
        pool_ = nullptr;
    }

    if (cache_data_ != nullptr) {
        delete[] cache_data_;
        cache_data_ = nullptr;

#ifdef LAMURE_ENABLE_INFO
        std::cout << "PLOD: ooc-cache shutdown" << std::endl;
#endif
    }

}

OocCache* OocCache::
GetInstance() {
    if (!is_instanced_) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!is_instanced_) {
            Policy* policy = Policy::GetInstance();
            ModelDatabase* database = ModelDatabase::GetInstance();
            size_t size_of_node_in_bytes = database->size_of_surfel() * database->surfels_per_node();
            size_t out_of_core_budget_in_nodes = (policy->out_of_core_budget_in_mb()*1024*1024) / size_of_node_in_bytes;
            single_ = new OocCache(out_of_core_budget_in_nodes);
            is_instanced_ = true;
        }

        return single_;
    }
    else {
        return single_;
    }
}

void OocCache::
RegisterNode(const model_t model_id, const node_t node_id, const int32_t priority) {
    if (IsNodeResident(model_id, node_id)) {
        return;
    }

    CacheQueue::QueryResult query_result = pool_->AcknowledgeQuery(model_id, node_id);

    switch (query_result) {
        case CacheQueue::QueryResult::NOT_INDEXED:
        {
            slot_t slot_id = index_->ReserveSlot();
            CacheQueue::Job job(model_id, node_id, slot_id, priority, cache_data_ + slot_id * slot_size());
            if (!pool_->AcknowledgeRequest(job)) {
                index_->UnreserveSlot(slot_id);
            }
            break;
        }

        case CacheQueue::QueryResult::INDEXED_AS_WAITING:
            pool_->AcknowledgeUpdate(model_id, node_id, priority);
            break;

        case CacheQueue::QueryResult::INDEXED_AS_LOADING:
            //note: this means the queue is either not updateable at all
            //or the node cannot be updated anymore, so we do nothing
            break;

        default: break;
    }

}

char* OocCache::
NodeData(const model_t model_id, const node_t node_id) {
    return cache_data_ + index_->GetSlot(model_id, node_id) * slot_size();

}

const bool OocCache::
IsNodeResidentAndAquired(const model_t model_id, const node_t node_id) {
    return index_->IsNodeAquired(model_id, node_id);

}

void OocCache::
Refresh() {
    pool_->Lock();
    pool_->ResolveCacheHistory(index_);


#ifdef LAMURE_CUT_UPDATE_ENABLE_CACHE_MAINTENANCE
    //if (!in_core_mode_)
    {
        ++maintenance_counter_;

        if (maintenance_counter_ > LAMURE_CUT_UPDATE_CACHE_MAINTENANCE_COUNTER) {
            pool_->PerformQueueMaintenance(index_);
            maintenance_counter_ = 0;
        }
    }
#endif


    pool_->Unlock();
}

void OocCache::
LockPool() {
    pool_->Lock();
    pool_->ResolveCacheHistory(index_);
}

void OocCache::
UnlockPool() {
    pool_->Unlock();

}

void OocCache::
StartMeasure() {
  pool_->StartMeasure();
}

void OocCache::
EndMeasure() {
  pool_->EndMeasure();
}


} // namespace ren

} // namespace lamure


