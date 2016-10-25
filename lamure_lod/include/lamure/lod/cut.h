// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_LOD_CUT_H_
#define LAMURE_LOD_CUT_H_

#include <lamure/types.h>
#include <lamure/platform_lod.h>
#include <lamure/lod/semaphore.h>
#include <lamure/lod/cache_queue.h>
#include <lamure/assert.h>

#include <vector>
#include <deque>

namespace lamure {
namespace lod {

class LOD_DLL cut
{
public:
                        cut();
                        cut(const context_t context_id, const view_t view_id, const model_t model_id);
    virtual             ~cut();

    struct node_slot_aggregate
    {
        node_slot_aggregate(
            const node_t node_id,
            const slot_t slot_id)
            : node_id_(node_id),
            slot_id_(slot_id) {};

        node_t          node_id_;
        slot_t          slot_id_;
    };

    const context_t     context_id() const { return context_id_; };
    const view_t        view_id() const { return view_id_; };
    const model_t       model_id() const { return model_id_; };

    std::vector<cut::node_slot_aggregate>& complete_set() { return complete_set_; };
    void                set_complete_set(std::vector<cut::node_slot_aggregate>& complete_set) { complete_set_ = complete_set; };


protected:

private:

    context_t           context_id_;
    view_t              view_id_;
    model_t             model_id_;

    std::vector<cut::node_slot_aggregate> complete_set_;
};


} } // namespace lamure


#endif // LAMURE_LOD_CUT_H_
