
namespace lamure {
namespace util {

template<typename item_t>
heap_base_t<item_t>::
heap_base_t()
: num_slots_(0) {

}

template<typename item_t>
heap_base_t<item_t>::
~heap_base_t() {
  slots_.clear();
  slot_map_.clear();
}

template<typename item_t>
const slot_id_t heap_base_t<item_t>::
get_num_slots() {
    std::lock_guard<std::mutex> lock(mutex_);
    return num_slots_;
}

template<typename item_t>
const bool heap_base_t<item_t>::
is_item_indexed(const unsigned int _item_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return (slot_map_.find(_item_id) != slot_map_.end());
}

template<typename item_t>
void heap_base_t<item_t>::
push(const item_t& _item) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (slot_map_.find(_item.id_) == slot_map_.end()) {
        slots_.push_back(_item);
        slot_map_[_item.id_] = num_slots_;

        ++num_slots_;

        shuffle_up(num_slots_-1);
    }
}

template<typename item_t>
const item_t heap_base_t<item_t>::
top() {
    std::lock_guard<std::mutex> lock(mutex_);

    item_t item;

    if (num_slots_ > 0) {
        item = slots_.front();
    }

    return item;
}

template<typename item_t>
void heap_base_t<item_t>::
pop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (num_slots_ > 0) {
        swap(0, num_slots_-1);
        unsigned int id = slots_.back().id_;
        slots_.pop_back();
        --num_slots_;
        shuffle_down(0);
        slot_map_.erase(id);
    }
}

template<typename item_t>
void heap_base_t<item_t>::
remove_all(const unsigned int _item_id, 
           std::vector<item_t>& _removed) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<slot_id_t> erase;

    for (const auto& it : slot_map_) {
        item_t& c = slots_[it.second];
        if (c.id0_ == _item_id || c.id1_ == _item_id) {
            _removed.push_back(c);
            erase.push_back(it.first);
        }
    }

    for (const auto& e : erase) {
        const auto it = slot_map_.find(e);

        if (it != slot_map_.end()) {
            slot_id_t slot_id = it->second;

            swap(slot_id, num_slots_-1);
            slots_.pop_back();
            --num_slots_;
            shuffle_down(slot_id);
            slot_map_.erase(e);

        }
    }
}

template<typename item_t>
void heap_base_t<item_t>::
swap(const slot_id_t _slot_id_0, 
     const slot_id_t _slot_id_1) {
    item_t& e0 = slots_[_slot_id_0];
    item_t& e1 = slots_[_slot_id_1];

    slot_map_[e0.id_] = _slot_id_1;
    slot_map_[e1.id_] = _slot_id_0;
    std::swap(slots_[_slot_id_0], slots_[_slot_id_1]);
}

template<typename item_t>
void heap_base_t<item_t>::
shuffle_up(const slot_id_t _slot_id) {
    if (_slot_id == 0) {
        return;
    }   

    slot_id_t parent_slot_id = (slot_id_t)std::floor((_slot_id-1)/2);

    if (slots_[_slot_id].cost_ > slots_[parent_slot_id].cost_) {
        return;
    }   

    swap(_slot_id, parent_slot_id);
    shuffle_up(parent_slot_id);
}

template<typename item_t>
void heap_base_t<item_t>::
shuffle_down(const slot_id_t _slot_id) {
    slot_id_t left_child_id = _slot_id*2 + 1;
    slot_id_t right_child_id = _slot_id*2 + 2;

    slot_id_t replace_id = _slot_id;

    if (right_child_id < num_slots_) {
        bool left_cheaper = slots_[right_child_id].cost_ > slots_[left_child_id].cost_;

        if (left_cheaper && slots_[_slot_id].cost_ > slots_[left_child_id].cost_) {
            replace_id = left_child_id;
        }
        else if (!left_cheaper && slots_[_slot_id].cost_ > slots_[right_child_id].cost_) {
            replace_id = right_child_id;
        }
    }   
    else if (left_child_id < num_slots_) {
        if (slots_[_slot_id].cost_ > slots_[left_child_id].cost_) {
            replace_id = left_child_id;
        }
    }   

    if (replace_id != _slot_id) {
        swap(_slot_id, replace_id);
        shuffle_down(replace_id);
    }   
}


}
}
