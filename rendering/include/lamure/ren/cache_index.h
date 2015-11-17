// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_CACHE_INDEX_H_
#define REN_CACHE_INDEX_H_

#include <lamure/types.h>
#include <lamure/utils.h>
#include <lamure/ren/config.h>
#include <lamure/ren/platform.h>

#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <mutex>
#include <iostream>


namespace lamure {
namespace ren {

class RENDERING_DLL CacheIndex
{
public:
                        CacheIndex(const model_t num_models, const slot_t num_slots);
    virtual             ~CacheIndex();

    const slot_t        num_slots() const { return num_slots_; };

    const slot_t        NumFreeSlots();
    const slot_t        ReserveSlot();
    void                ApplySlot(const slot_t slot_id, const model_t model_id, const node_t node_id);
    void                UnreserveSlot(const slot_t slot_id);

    const slot_t        GetSlot(const model_t model_id, const node_t node_id);
    const bool          IsNodeIndexed(const model_t model_id, const node_t node_id);
    const bool          IsNodeAquired(const model_t model_id, const node_t node_id);

    void                AquireSlot(const view_t view_id, const model_t model_id, const node_t node_id);
    void                ReleaseSlot(const view_t view_id, const model_t model_id, const node_t node_id);
    const bool          ReleaseSlotInvalidate(const view_t view_id, const model_t model_id, const node_t node_id);

private:

    model_t             num_models_;
    slot_t              num_slots_;
    slot_t              num_free_slots_;


    struct CacheIndexNode
    {
        CacheIndexNode(
            const model_t model_id,
            const node_t node_id,
            const slot_t prev,
            const slot_t next)
            : model_id_(model_id),
            node_id_(node_id),
            prev_(prev),
            next_(next) {};

        CacheIndexNode()
            : model_id_(invalid_model_t),
            node_id_(invalid_node_t),
            prev_(invalid_slot_t),
            next_(invalid_slot_t) {};

        model_t         model_id_;
        node_t          node_id_;
        slot_t          prev_;
        slot_t          next_;
        std::set<view_t> views_;
    };

    std::mutex          mutex_;

    std::vector<CacheIndexNode> slots_;
    std::vector<std::map<node_t, slot_t>> maps_;
};



} } // namespace lamure


#endif // REN_CACHE_INDEX_H_

