// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/cut_update_index.h>

namespace lamure
{

namespace ren
{


CutUpdateIndex::
CutUpdateIndex()
: num_views_(0),
  num_models_(0),
  current_cut_front_(CutFront::FRONT_A) {

    ModelDatabase* database = ModelDatabase::GetInstance();

    num_models_ = database->num_models();

    for (int32_t queue_id = 0; queue_id < Queue::NUM_QUEUES; ++queue_id) {
        num_slots_[queue_id] = 0;
        slot_maps_[queue_id].resize(num_models_);
    }

    for (const auto& view_id : view_ids_) {
        front_a_cuts_[view_id].resize(num_models_);
        front_b_cuts_[view_id].resize(num_models_);
    }

    for (model_t model_id = 0; model_id < num_models_; ++model_id) {
        fan_factor_table_.push_back(database->GetModel(model_id)->bvh()->fan_factor());
        num_nodes_table_.push_back(database->GetModel(model_id)->bvh()->num_nodes());
    }

}

CutUpdateIndex::
~CutUpdateIndex() {

}

void CutUpdateIndex::
UpdatePolicy(const view_t num_views) {
    std::lock_guard<std::mutex> lock(mutex_);

    view_t prev_num_views = num_views_;
    num_views_ = num_views;

    if (num_views_ > prev_num_views) {
        view_ids_.clear();
        for (view_t view_id = 0; view_id < num_views; ++view_id) {
            view_ids_.insert(view_id);
        }
    }

    ModelDatabase* database = ModelDatabase::GetInstance();

    if (database->num_models() != num_models_) {
        num_models_ = database->num_models();

        for (int32_t queue_id = 0; queue_id < Queue::NUM_QUEUES; ++queue_id) {
            num_slots_[queue_id] = 0;
            slot_maps_[queue_id].clear();
            slot_maps_[queue_id].resize(num_models_);
        }

        for (const auto& view_id : view_ids_) {
            front_a_cuts_[view_id].clear();
            front_a_cuts_[view_id].resize(num_models_);
            front_b_cuts_[view_id].clear();
            front_b_cuts_[view_id].resize(num_models_);
        }

        fan_factor_table_.clear();
        num_nodes_table_.clear();

        for (model_t model_id = 0; model_id < num_models_; ++model_id) {
            fan_factor_table_.push_back(database->GetModel(model_id)->bvh()->fan_factor());
            num_nodes_table_.push_back(database->GetModel(model_id)->bvh()->num_nodes());
        }

    }
    else if (num_views_ > prev_num_views) {
        for (const auto& view_id : view_ids_) {
            front_a_cuts_[view_id].resize(num_models_);
            front_b_cuts_[view_id].resize(num_models_);
        }
    }


}

const node_t CutUpdateIndex::
NumNodes(const model_t model_id) const {
    assert(model_id < num_models_);

    return num_nodes_table_[model_id];
}

const size_t CutUpdateIndex::
FanFactor(const model_t model_id) const {
    assert(model_id < num_models_);

    return fan_factor_table_[model_id];
}

const size_t CutUpdateIndex::
NumActions(const Queue queue) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(queue < Queue::NUM_QUEUES);

    return num_slots_[queue];
}

const std::set<node_t>& CutUpdateIndex::
GetCurrentCut(const view_t view_id, const model_t model_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(view_ids_.find(view_id) != view_ids_.end());
    assert(model_id < num_models_);

    switch (current_cut_front_) {
        case CutFront::FRONT_A:
            return front_a_cuts_[view_id][model_id];
            break;

        case CutFront::FRONT_B:
            return front_b_cuts_[view_id][model_id];
            break;

        default: break;

    }

    return front_a_cuts_[view_id][model_id];
}

const std::set<node_t>& CutUpdateIndex::
GetPreviousCut(const view_t view_id, const model_t model_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(view_ids_.find(view_id) != view_ids_.end());
    assert(model_id < num_models_);

    switch (current_cut_front_) {
        case CutFront::FRONT_A:
            return front_b_cuts_[view_id][model_id];
            break;

        case CutFront::FRONT_B:
            return front_a_cuts_[view_id][model_id];
            break;

        default: break;

    }

    return front_b_cuts_[view_id][model_id];
}

void CutUpdateIndex::
SwapCuts() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (current_cut_front_ == CutFront::FRONT_A) {
        current_cut_front_ = CutFront::FRONT_B;
    }
    else {
        current_cut_front_ = CutFront::FRONT_A;
    }

}

void CutUpdateIndex::
ResetCut(const view_t view_id, const model_t model_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(view_ids_.find(view_id) != view_ids_.end());
    assert(model_id < num_models_);

    switch (current_cut_front_) {
        case CutFront::FRONT_A:
            front_a_cuts_[view_id][model_id].clear();
            break;

        case CutFront::FRONT_B:
            front_b_cuts_[view_id][model_id].clear();
            break;

        default: break;

    }

}

void CutUpdateIndex::
PushAction(const Action& action, bool sort) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(action.model_id_ < num_models_);
    assert(action.node_id_ < num_nodes_table_[action.model_id_]);
    assert(action.queue_ < Queue::NUM_QUEUES);

    AddAction(action, sort);
}

const CutUpdateIndex::Action CutUpdateIndex::
FrontAction(const Queue queue) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(queue < Queue::NUM_QUEUES);

    Action action;

    if (num_slots_[queue] > 0) {
        action = slots_[queue].front();
    }

    return action;
}

const CutUpdateIndex::Action CutUpdateIndex::
BackAction(const Queue queue) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(queue < Queue::NUM_QUEUES);

    Action action;

    if (num_slots_[queue] > 0) {
        action = slots_[queue].back();
    }

    return action;
}

void CutUpdateIndex::
PopFrontAction(const Queue queue) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(queue < Queue::NUM_QUEUES);
    assert(!slots_[queue].empty());

    Action action = slots_[queue].front();
    assert(action.queue_ == queue);

    Swap(queue, 0, num_slots_[queue]-1);


    assert(slot_maps_[queue][action.model_id_][action.node_id_].find(num_slots_[queue]-1) != slot_maps_[queue][action.model_id_][action.node_id_].end());

    slots_[queue].pop_back();
    slot_maps_[queue][action.model_id_][action.node_id_].erase(num_slots_[queue]-1);

    --num_slots_[queue];

    ShuffleDown(queue, 0);

    if (slot_maps_[queue][action.model_id_][action.node_id_].empty()) {
        slot_maps_[queue][action.model_id_].erase(action.node_id_);
    }

}

void CutUpdateIndex::
PopBackAction(const Queue queue) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(queue < Queue::NUM_QUEUES);
    assert(!slots_[queue].empty());

    Action action = slots_[queue].back();
    assert(action.queue_ == queue);

    assert(slot_maps_[queue][action.model_id_][action.node_id_].find(num_slots_[queue]-1) != slot_maps_[queue][action.model_id_][action.node_id_].end());


    slots_[queue].pop_back();
    slot_maps_[queue][action.model_id_][action.node_id_].erase(num_slots_[queue]-1);

    --num_slots_[queue];

    if (slot_maps_[queue][action.model_id_][action.node_id_].empty()) {
        slot_maps_[queue][action.model_id_].erase(action.node_id_);
    }

}

void CutUpdateIndex::
ApproveAction(const Action& action) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(action.model_id_ < num_models_);
    assert(action.node_id_ < num_nodes_table_[action.model_id_]);
    assert(action.queue_ < Queue::NUM_QUEUES);

    //approve action, this adds the action to all cuts of all the users in question.
    switch (action.queue_) {
        case Queue::KEEP:
            switch (current_cut_front_) {
                case CutFront::FRONT_A:
                    front_a_cuts_[action.view_id_][action.model_id_].insert(action.node_id_);
                    break;

                case CutFront::FRONT_B:
                    front_b_cuts_[action.view_id_][action.model_id_].insert(action.node_id_);
                    break;

                default: break;

            }
            break;

        case Queue::MUST_SPLIT:
            {
                std::vector<node_t> children;
                GetAllChildren(action.model_id_, action.node_id_, children);

                switch (current_cut_front_) {
                    case CutFront::FRONT_A:
                        front_a_cuts_[action.view_id_][action.model_id_].insert(children.begin(), children.end());
                        break;

                    case CutFront::FRONT_B:
                        front_b_cuts_[action.view_id_][action.model_id_].insert(children.begin(), children.end());
                        break;

                    default: break;

                }

            }
            break;

        case Queue::MUST_COLLAPSE:
            switch (current_cut_front_) {
                case CutFront::FRONT_A:
                    front_a_cuts_[action.view_id_][action.model_id_].insert(action.node_id_);
                    break;

                case CutFront::FRONT_B:
                    front_b_cuts_[action.view_id_][action.model_id_].insert(action.node_id_);
                    break;

                default: break;

            }
            break;

        case Queue::COLLAPSE_ON_NEED:
            //if a collapse-on-need-action is approved, we collapse the node
            switch (current_cut_front_) {
                case CutFront::FRONT_A:
                    front_a_cuts_[action.view_id_][action.model_id_].insert(action.node_id_);
                    break;

                case CutFront::FRONT_B:
                    front_b_cuts_[action.view_id_][action.model_id_].insert(action.node_id_);
                    break;

                default: break;

            }
            break;


        case Queue::MAYBE_COLLAPSE:
            //if a maybe-collapse-action is approved, we collapse the node
            switch (current_cut_front_) {
                case CutFront::FRONT_A:
                    front_a_cuts_[action.view_id_][action.model_id_].insert(action.node_id_);
                    break;

                case CutFront::FRONT_B:
                    front_b_cuts_[action.view_id_][action.model_id_].insert(action.node_id_);
                    break;

                default: break;

            }
            break;

        default: break;
    }

}

void CutUpdateIndex::
RejectAction(const Action& action) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(action.model_id_ < num_models_);
    assert(action.node_id_ < num_nodes_table_[action.model_id_]);
    assert(action.queue_ < Queue::NUM_QUEUES);

    //raise replacement action
    switch (action.queue_) {
        case Queue::KEEP:
            assert(false);
            break;

        case Queue::MUST_SPLIT:
            switch (current_cut_front_) {
                case CutFront::FRONT_A:
                    front_a_cuts_[action.view_id_][action.model_id_].insert(action.node_id_);
                    break;

                case CutFront::FRONT_B:
                    front_b_cuts_[action.view_id_][action.model_id_].insert(action.node_id_);
                    break;

                default: break;

            }
            break;

        case Queue::MUST_COLLAPSE: {
                std::vector<node_t> children;
                GetAllChildren(action.model_id_, action.node_id_, children);

                switch (current_cut_front_) {
                    case CutFront::FRONT_A:
                        front_a_cuts_[action.view_id_][action.model_id_].insert(children.begin(), children.end());
                        break;

                    case CutFront::FRONT_B:
                        front_b_cuts_[action.view_id_][action.model_id_].insert(children.begin(), children.end());
                        break;

                    default: break;

                }
            }
            break;

        case Queue::COLLAPSE_ON_NEED: {
                std::vector<node_t> children;
                GetAllChildren(action.model_id_, action.node_id_, children);

                switch (current_cut_front_) {
                    case CutFront::FRONT_A:
                        front_a_cuts_[action.view_id_][action.model_id_].insert(children.begin(), children.end());
                        break;

                    case CutFront::FRONT_B:
                        front_b_cuts_[action.view_id_][action.model_id_].insert(children.begin(), children.end());
                        break;

                    default: break;

                }
            }
            break;

        case Queue::MAYBE_COLLAPSE:
            {
                std::vector<node_t> children;
                GetAllChildren(action.model_id_, action.node_id_, children);

                switch (current_cut_front_) {
                    case CutFront::FRONT_A:
                        front_a_cuts_[action.view_id_][action.model_id_].insert(children.begin(), children.end());
                        break;

                    case CutFront::FRONT_B:
                        front_b_cuts_[action.view_id_][action.model_id_].insert(children.begin(), children.end());
                        break;

                    default: break;

                }
            }
            break;

        default: break;
    }

}

void CutUpdateIndex::
AddAction(const Action& action, bool sort) {
    assert(action.model_id_ < num_models_);
    assert(action.node_id_ < num_nodes_table_[action.model_id_]);
    assert(action.queue_ < Queue::NUM_QUEUES);

    if (sort) {
        slots_[action.queue_].push_back(action);
        ++num_slots_[action.queue_];

        slot_maps_[action.queue_][action.model_id_][action.node_id_].insert(num_slots_[action.queue_]-1);

        ShuffleUp(action.queue_, num_slots_[action.queue_]-1);

    }
    else {
        initial_queue_.push(action);
    }

}

void CutUpdateIndex::
CancelAction(const view_t view_id, const model_t model_id, const node_t node_id) {
    assert(model_id < num_models_);
    assert(node_id < num_nodes_table_[model_id]);
    assert(view_ids_.find(view_id) != view_ids_.end());

    //firstly, cancel actions that already happened (remove nodes from cuts)

    switch (current_cut_front_) {
        case CutFront::FRONT_A:
            if (front_a_cuts_[view_id][model_id].find(node_id) != front_a_cuts_[view_id][model_id].end()) {
                front_a_cuts_[view_id][model_id].erase(node_id);
            }
            break;

        case CutFront::FRONT_B:
            if (front_b_cuts_[view_id][model_id].find(node_id) != front_b_cuts_[view_id][model_id].end()) {
                front_b_cuts_[view_id][model_id].erase(node_id);
            }
            break;

        default: break;

    }

    //secondly, cancel all pending actions (remove actions from queues)

    for (uint32_t queue = 0; queue < Queue::NUM_QUEUES; ++queue) {
        const auto slot_it = slot_maps_[queue][model_id].find(node_id);

        if (slot_it == slot_maps_[queue][model_id].end()) {
            continue;
        }

        assert(slot_it != slot_maps_[queue][model_id].end());

        std::set<slot_t> slot_ids = slot_it->second; //copy

        std::set<slot_t>::iterator it = std::prev(slot_ids.end());

        while (true) {
            std::set<slot_t>::iterator current_it = it--;

            slot_t slot_id = *(current_it);

            if (slots_[queue][slot_id].view_id_ == view_id) {

                Action current_item = slots_[queue][slot_id];
                Action last_item = slots_[queue][num_slots_[queue]-1];

                assert(slot_maps_[queue][current_item.model_id_][current_item.node_id_].find(slot_id) != slot_maps_[queue][current_item.model_id_][current_item.node_id_].end());
                assert(slot_maps_[queue][last_item.model_id_][last_item.node_id_].find(num_slots_[queue]-1) != slot_maps_[queue][last_item.model_id_][last_item.node_id_].end());

                assert(current_item.queue_ == queue);
                assert(current_item.model_id_ == model_id);
                assert(current_item.node_id_ == node_id);
                assert(last_item.queue_ == queue);

                slot_maps_[queue][current_item.model_id_][current_item.node_id_].erase(slot_id);
                slot_maps_[queue][last_item.model_id_][last_item.node_id_].erase(num_slots_[queue]-1);

                slot_maps_[queue][current_item.model_id_][current_item.node_id_].insert(num_slots_[queue]-1);
                slot_maps_[queue][last_item.model_id_][last_item.node_id_].insert(slot_id);

                std::swap(slots_[queue][slot_id], slots_[queue][num_slots_[queue]-1]);

                slot_maps_[queue][current_item.model_id_][current_item.node_id_].erase(num_slots_[queue]-1);

                slots_[queue].pop_back();

                --num_slots_[queue];

                ShuffleDown((Queue)queue, slot_id);

            }

            if (current_it == slot_ids.begin()) {
                break;
            }
        }

        if (slot_maps_[queue][model_id][node_id].empty()) {
            slot_maps_[queue][model_id].erase(node_id);
        }
    }

}

void CutUpdateIndex::
Sort() {
    while(!initial_queue_.empty()) {
        Action action = initial_queue_.top();
        initial_queue_.pop();

        slots_[action.queue_].push_back(action);
        ++num_slots_[action.queue_];

        slot_maps_[action.queue_][action.model_id_][action.node_id_].insert(num_slots_[action.queue_]-1);

        ShuffleUp(action.queue_, num_slots_[action.queue_]-1);

    }

}

void CutUpdateIndex::
Swap(const Queue queue, const size_t slot_id_0, const size_t slot_id_1) {
    if (slot_id_0 == slot_id_1) {
        return;
    }

    Action& item0 = slots_[queue][slot_id_0];
    Action& item1 = slots_[queue][slot_id_1];


    assert(slot_maps_[queue][item0.model_id_][item0.node_id_].find(slot_id_0) != slot_maps_[queue][item0.model_id_][item0.node_id_].end());
    assert(slot_maps_[queue][item1.model_id_][item1.node_id_].find(slot_id_1) != slot_maps_[queue][item1.model_id_][item1.node_id_].end());


    slot_maps_[queue][item0.model_id_][item0.node_id_].erase(slot_id_0);
    slot_maps_[queue][item1.model_id_][item1.node_id_].erase(slot_id_1);

    slot_maps_[queue][item0.model_id_][item0.node_id_].insert(slot_id_1);
    slot_maps_[queue][item1.model_id_][item1.node_id_].insert(slot_id_0);

    std::swap(slots_[queue][slot_id_0], slots_[queue][slot_id_1]);
}

void CutUpdateIndex::
ShuffleUp(const Queue queue, const size_t slot_id) {
    if (slot_id == 0) {
        return;
    }

    size_t parent_slot_id = (size_t)std::floor((slot_id-1)/2);

    if (slots_[queue][slot_id].error_ < slots_[queue][parent_slot_id].error_) {
        return;
    }

    Swap(queue, slot_id, parent_slot_id);

    ShuffleUp(queue, parent_slot_id);
}

void CutUpdateIndex::
ShuffleDown(const Queue queue, const size_t slot_id) {
    size_t left_child_id = slot_id*2 + 1;
    size_t right_child_id = slot_id*2 + 2;

    size_t replace_id = slot_id;

    if (slot_id == num_slots_[queue]-1) {
        return;
    }

    if (right_child_id < num_slots_[queue]) {
        bool left_greater = slots_[queue][right_child_id].error_ < slots_[queue][left_child_id].error_;

        if (left_greater && slots_[queue][slot_id].error_ < slots_[queue][left_child_id].error_) {
            replace_id = left_child_id;
        }
        else if (!left_greater && slots_[queue][slot_id].error_ < slots_[queue][right_child_id].error_) {
            replace_id = right_child_id;
        }

    }
    else if (left_child_id < num_slots_[queue]) {
        if (slots_[queue][slot_id].error_ < slots_[queue][left_child_id].error_) {
            replace_id = left_child_id;
        }
    }

    if (replace_id != slot_id) {
        Swap(queue, slot_id, replace_id);
        ShuffleDown(queue, replace_id);
    }
}

const node_t CutUpdateIndex::
GetChildId(const model_t model_id, const node_t node_id, const node_t child_index) const {
    assert(model_id < num_models_);
    assert(node_id < num_nodes_table_[model_id]);
    assert(child_index < fan_factor_table_[model_id]);

    uint32_t fan_factor = fan_factor_table_[model_id];
    node_t num_nodes = num_nodes_table_[model_id];

    node_t child_id = node_id*fan_factor + 1 + child_index;

    if (child_id < num_nodes) {
        return child_id;
    }
    else {
        return invalid_node_t;
    }
}

const node_t CutUpdateIndex::
GetParentId(const model_t model_id, const node_t node_id) const {
    assert(model_id < num_models_);
    assert(node_id < num_nodes_table_[model_id]);

    if (node_id == 0) {
        return invalid_node_t;
    }

    uint32_t fan_factor = fan_factor_table_[model_id];

    if (node_id % (node_t)fan_factor == 0) {
        return node_id/fan_factor - 1;
    }
    else {
        return (node_id + fan_factor - (node_id % (node_t)fan_factor)) / fan_factor - 1;
    }
}

void CutUpdateIndex::
GetAllSiblings(const model_t model_id, const node_t node_id, std::vector<node_t>& siblings) const {
    assert(model_id < num_models_);
    assert(node_id < num_nodes_table_[model_id]);

    node_t parent_id = GetParentId(model_id, node_id);

    if (parent_id == invalid_node_t) {
        return;
    }

    uint32_t fan_factor = fan_factor_table_[model_id];

    for (node_t i = 0; i < (node_t)fan_factor; ++i) {
        siblings.push_back(GetChildId(model_id, parent_id, i));
    }
}

void CutUpdateIndex::
GetAllChildren(const model_t model_id, const node_t node_id, std::vector<node_t>& children) const {
    assert(model_id < num_models_);
    assert(node_id < num_nodes_table_[model_id]);

    uint32_t fan_factor = fan_factor_table_[model_id];

    for (node_t i = 0; i < (node_t)fan_factor; ++i) {
        children.push_back(GetChildId(model_id, node_id, i));
    }
}


} // namespace ren

} // namespace lamure



