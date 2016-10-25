// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_LOD_GPU_CACHE_H_
#define LAMURE_LOD_GPU_CACHE_H_

#include <lamure/types.h>
#include <lamure/assert.h>
#include <lamure/lod/cache.h>
#include <lamure/platform_lod.h>

#include <unordered_set>

namespace lamure {
namespace lod {

class LOD_DLL gpu_cache : public cache
{
public:

                        gpu_cache(const slot_t num_slots);
    virtual             ~gpu_cache();

    const node_t        transfer_budget() const { return transfer_budget_; };
    void                set_transfer_budget(const node_t transfer_budget) { transfer_budget_ = transfer_budget; };

    const node_t        transfer_slots_written() const { return transfer_slots_written_; };
    void                set_transfer_slots_written(const node_t transfer_slots_written) { transfer_slots_written_ = transfer_slots_written; };

    const std::vector<std::unordered_set<node_t>>& transfer_list() const { return transfer_list_; };

    const bool          register_node(const model_t model_id, const node_t node_id);

    void                reset_transfer_list();
    void                remove_from_transfer_list(const model_t model_id, const node_t node_id);

private:
    node_t              transfer_budget_;
    node_t              transfer_slots_written_;
    std::vector<std::unordered_set<node_t>> transfer_list_;

};


} } // namespace lamure


#endif // LAMURE_LOD_GPU_CACHE_H_
