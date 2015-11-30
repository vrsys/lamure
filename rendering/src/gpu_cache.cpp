// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/gpu_cache.h>

namespace lamure {
namespace ren {

GpuCache::
GpuCache(const slot_t num_slots)
    : Cache(num_slots),
    transfer_budget_(0),
    transfer_slots_written_(0) {
    Modeldatabase* database = Modeldatabase::get_instance();
    transfer_list_.resize(database->num_models());
}

GpuCache::
~GpuCache() {
    transfer_list_.clear();
}

void GpuCache::
resetTransferList() {
    Modeldatabase* database = Modeldatabase::get_instance();
    transfer_list_.clear();
    transfer_list_.resize(database->num_models());
}

const bool GpuCache::
RegisterNode(const model_t model_id, const node_t node_id) {
    if (IsNodeResident(model_id, node_id)) {
        return false;
    }

    if (transfer_budget_ > 0) {
        --transfer_budget_;
    }

    node_t least_recently_used_slot = index_->ReserveSlot();

    index_->ApplySlot(least_recently_used_slot, model_id, node_id);

    transfer_list_[model_id].insert(node_id);

    return true;
}


void GpuCache::
RemoveFromTransferList(const model_t model_id, const node_t node_id) {
    if (transfer_list_[model_id].find(node_id) != transfer_list_[model_id].end()) {
        transfer_list_[model_id].erase(node_id);
        ++transfer_budget_;
    }
}



} } // namespace lamure
