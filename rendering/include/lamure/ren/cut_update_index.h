// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_CUT_UPDATE_INDEX_H_
#define REN_CUT_UPDATE_INDEX_H_

#include <lamure/types.h>
#include <lamure/utils.h>
#include <lamure/ren/config.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <set>
#include <map>
#include <mutex>
#include <assert.h>
#include <algorithm>
#include <stack>

#include <lamure/ren/model_database.h>



namespace lamure {
namespace ren {


class CutUpdateIndex
{
public:

    enum Queue
    {
        KEEP = 0,
        MUST_SPLIT = 1,
        MUST_COLLAPSE = 2,
        COLLAPSE_ON_NEED = 3,
        MAYBE_COLLAPSE = 4,
        NUM_QUEUES = 5
    };

    struct Action
    {
        explicit Action(
            const Queue queue,
            const view_t view_id,
            const model_t model_id,
            const node_t node_id,
            const float error)
            : queue_(queue),
            view_id_(view_id),
            model_id_(model_id),
            node_id_(node_id),
            error_(error)
        {};

        explicit Action()
            : queue_(Queue::NUM_QUEUES),
            model_id_(invalid_model_t),
            node_id_(invalid_node_t),
            error_(0.f) {};

        Queue           queue_;
        view_t          view_id_;
        model_t         model_id_;
        node_t          node_id_;
        float           error_;
    };

    struct ActionCompare
    {
        bool operator() (const Action& l, const Action& r) const
        {
            return l.error_ < r.error_;
        }
    };

                        CutUpdateIndex();
    virtual             ~CutUpdateIndex();

    inline const view_t num_views() const { return num_views_; };
    inline const model_t num_models() const { return num_models_; };
    inline const std::set<view_t>& view_ids() const { return view_ids_; };

    void                UpdatePolicy(const view_t num_views);

    const node_t        NumNodes(const model_t model_id) const;
    const size_t        FanFactor(const model_t model_id) const;
    const size_t        NumActions(const Queue queue);

    void                PushAction(const Action& action, bool sort);
    const Action        FrontAction(const Queue queue);
    const Action        BackAction(const Queue queue);
    void                PopFrontAction(const Queue queue);
    void                PopBackAction(const Queue queue);

    const std::set<node_t>& GetCurrentCut(const view_t view_id, const model_t model_id);
    const std::set<node_t>& GetPreviousCut(const view_t view_id, const model_t model_id);
    void                SwapCuts();
    void                ResetCut(const view_t view_id, const model_t model_id);

    void                CancelAction(const view_t view_id, const model_t model_id, const node_t node_id);
    void                ApproveAction(const Action& action);
    void                RejectAction(const Action& action);

    const node_t        GetChildId(const model_t model_id, const node_t node_id, const node_t child_index) const;
    const node_t        GetParentId(const model_t model_id, const node_t node_id) const;
    void                GetAllSiblings(const model_t model_id, const node_t node_id, std::vector<node_t>& siblings) const;
    void                GetAllChildren(const model_t model_id, const node_t node_id, std::vector<node_t>& children) const;

    void                Sort();

private:

    enum CutFront
    {
        FRONT_A = 0,
        FRONT_B = 1,
        INVALID_FRONT = 2
    };

    void                AddAction(const Action& action, bool sort);

    void                Swap(const Queue queue, const size_t slot_id_0, const size_t slot_id_1);
    void                ShuffleUp(const Queue queue, const size_t slot_id);
    void                ShuffleDown(const Queue queue, const size_t slot_id);

    size_t              num_slots_[Queue::NUM_QUEUES];
    view_t              num_views_;
    model_t             num_models_;
    std::mutex          mutex_;

    std::vector<uint32_t> fan_factor_table_;
    std::vector<node_t> num_nodes_table_;
    std::set<view_t> view_ids_;

    std::vector<Action> slots_[Queue::NUM_QUEUES];
    std::stack<Action> initial_queue_;

    //mapping [queue] (model, node) to slot
    std::vector<std::map<node_t, std::set<slot_t>>> slot_maps_[Queue::NUM_QUEUES];

    CutFront            current_cut_front_;
    //[user][model][node]
    std::map<view_t, std::vector<std::set<node_t>>> front_a_cuts_;
    std::map<view_t, std::vector<std::set<node_t>>> front_b_cuts_;

};



} } // namespace lamure


#endif // REN_CUT_UPDATE_INDEX_H_
