// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/cache.h>


namespace lamure
{

namespace ren
{

Cache::
Cache(const slot_t num_slots)
    : num_slots_(num_slots), slot_size_(0) {
    ModelDatabase* database = ModelDatabase::GetInstance();

    slot_size_ = (size_t)(database->size_of_surfel() * database->surfels_per_node());

    index_ = new CacheIndex(database->num_models(), num_slots_);
}

Cache::
~Cache() {
    if (index_ != nullptr) {
        delete index_;
        index_ = nullptr;
    }
}

const bool Cache::
IsNodeResident(const model_t model_id, const node_t node_id) {
    return index_->IsNodeIndexed(model_id, node_id);
}

const slot_t Cache::
NumFreeSlots() {
    return index_->NumFreeSlots();
}

const slot_t Cache::
SlotId(const model_t model_id, const node_t node_id) {
    return index_->GetSlot(model_id, node_id);
}

void Cache::
AquireNode(const context_t context_id, const view_t view_id, const model_t model_id, const node_t node_id) {
    if (index_->IsNodeIndexed(model_id, node_id)) {
        uint32_t hash_id = ((((uint32_t)context_id) & 0xFFFF) << 16) | (((uint32_t)view_id) & 0xFFFF);
        index_->AquireSlot(hash_id, model_id, node_id);
    }
}

void Cache::
ReleaseNode(const context_t context_id, const view_t view_id, const model_t model_id, const node_t node_id) {
    if (index_->IsNodeIndexed(model_id, node_id)) {
        uint32_t hash_id = ((((uint32_t)context_id) & 0xFFFF) << 16) | (((uint32_t)view_id) & 0xFFFF);
        index_->ReleaseSlot(hash_id, model_id, node_id);
    }

}

const bool Cache::
ReleaseNodeInvalidate(const context_t context_id, const view_t view_id, const model_t model_id, const node_t node_id) {
    if (index_->IsNodeIndexed(model_id, node_id)) {
        uint32_t hash_id = ((((uint32_t)context_id) & 0xFFFF) << 16) | (((uint32_t)view_id) & 0xFFFF);
        return index_->ReleaseSlotInvalidate(hash_id, model_id, node_id);
    }

    return false;
}

void Cache::
Lock() {
    mutex_.lock();
}

void Cache::
Unlock() {
    mutex_.unlock();
}


} // namespace ren

} // namespace lamure


