// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/cache_queue.h>

namespace lamure
{

namespace ren
{

CacheQueue::
CacheQueue()
: num_slots_(0),
  num_models_(0),
  mode_(UpdateMode::UPDATE_NEVER),
  initialized_(false) {

}

CacheQueue::
~CacheQueue() {

}

const size_t CacheQueue::
NumJobs() {
    std::lock_guard<std::mutex> lock(mutex_);
    return num_slots_;
}

const CacheQueue::QueryResult CacheQueue::
IsNodeIndexed(const model_t model_id, const node_t node_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(initialized_);
    assert(model_id < num_models_);

    QueryResult result = QueryResult::NOT_INDEXED;

    if (requested_set_[model_id].find(node_id) != requested_set_[model_id].end()) {
        result = QueryResult::INDEXED_AS_LOADING;

        if (mode_ != UpdateMode::UPDATE_NEVER) {
            if (pending_set_[model_id].find(node_id) == pending_set_[model_id].end()) {
                result = QueryResult::INDEXED_AS_WAITING;
            }
        }
    }

    return result;
}

void CacheQueue::
Initialize(const UpdateMode mode, const model_t num_models) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(!initialized_);

    mode_ = mode;
    num_models_ = num_models;

    requested_set_.resize(num_models_);

    if (mode_ != UpdateMode::UPDATE_NEVER) {
        pending_set_.resize(num_models_);
    }

    initialized_ = true;
}

bool CacheQueue::
push_job(const Job& job) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(initialized_);
    assert(job.model_id_ < num_models_);

    if (requested_set_[job.model_id_].find(job.node_id_) == requested_set_[job.model_id_].end()) {
        slots_.push_back(job);
        requested_set_[job.model_id_][job.node_id_] = num_slots_;

        ++num_slots_;

        ShuffleUp(num_slots_-1);

        return true;
    }

    return false;

}

const CacheQueue::Job CacheQueue::
TopJob() {
    std::lock_guard<std::mutex> lock(mutex_);

    Job job;

    if (num_slots_ > 0) {
        job = slots_.front();

        if (mode_ != UpdateMode::UPDATE_NEVER) {
            pending_set_[job.model_id_].insert(job.node_id_);
        }

        Swap(0, num_slots_-1);
        slots_.pop_back();

        --num_slots_;

        ShuffleDown(0);
    }

    return job;
}

void CacheQueue::
pop_job(const Job& job) {
    std::lock_guard<std::mutex> lock(mutex_);

    assert(job.model_id_ < num_models_);

    requested_set_[job.model_id_].erase(job.node_id_);

    if (mode_ != UpdateMode::UPDATE_NEVER) {
        assert(pending_set_[job.model_id_].find(job.node_id_) != pending_set_[job.model_id_].end());

        if (pending_set_[job.model_id_].find(job.node_id_) != pending_set_[job.model_id_].end()) {
            pending_set_[job.model_id_].erase(job.node_id_);
        }
    }
}

void CacheQueue::
UpdateJob(const model_t model_id, const node_t node_id, int32_t priority) {
    if (mode_ == UpdateMode::UPDATE_NEVER) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    assert(model_id < num_models_);

    if (pending_set_[model_id].find(node_id) != pending_set_[model_id].end()) {
        return;
    }

    const auto it = requested_set_[model_id].find(node_id);

    //assert(it != requested_set_[model_id].end());

    if (it == requested_set_[model_id].end()) {
        return;
    }

    size_t slot_id = it->second;

    if (priority < slots_[slot_id].priority_) {
        if (mode_ == UpdateMode::UPDATE_ALWAYS || mode_ == UpdateMode::UPDATE_DECREMENT_ONLY) {
            slots_[slot_id].priority_ = priority;
            ShuffleDown(slot_id);
        }
    }
    else if (priority > slots_[slot_id].priority_) {
        if (mode_ == UpdateMode::UPDATE_ALWAYS || mode_ == UpdateMode::UPDATE_INCREMENT_ONLY) {
            slots_[slot_id].priority_ = priority;
            ShuffleUp(slot_id);
        }
    }

}

const CacheQueue::AbortResult CacheQueue::
AbortJob(const Job& job) {
    AbortResult abort_result = AbortResult::ABORT_FAILED;

    if (mode_ != UpdateMode::UPDATE_NEVER) {
        std::lock_guard<std::mutex> lock(mutex_);

        const auto it = requested_set_[job.model_id_].find(job.node_id_);

        if (it != requested_set_[job.model_id_].end()) {
            if (pending_set_[job.model_id_].find(job.node_id_) == pending_set_[job.model_id_].end()) {
                size_t slot_id = it->second;

                Swap(slot_id, num_slots_-1);
                slots_.pop_back();
                --num_slots_;
                ShuffleDown(slot_id);
                requested_set_[job.model_id_].erase(job.node_id_);

                abort_result = AbortResult::ABORT_SUCCESS;
            }
        }
    }

    return abort_result;
}

void CacheQueue::
Swap(const size_t slot_id_0, const size_t slot_id_1) {
    Job& job0 = slots_[slot_id_0];
    Job& job1 = slots_[slot_id_1];

    requested_set_[job0.model_id_][job0.node_id_] = slot_id_1;
    requested_set_[job1.model_id_][job1.node_id_] = slot_id_0;
    std::swap(slots_[slot_id_0], slots_[slot_id_1]);
}

void CacheQueue::
ShuffleUp(const size_t slot_id) {
    if (slot_id == 0) {
        return;
    }

    size_t parent_slot_id = (size_t)std::floor((slot_id-1)/2);

    if (slots_[slot_id].priority_ < slots_[parent_slot_id].priority_) {
        return;
    }

    Swap(slot_id, parent_slot_id);

    ShuffleUp(parent_slot_id);
}

void CacheQueue::
ShuffleDown(const size_t slot_id) {
    size_t left_child_id = slot_id*2 + 1;
    size_t right_child_id = slot_id*2 + 2;

    size_t replace_id = slot_id;

    if (right_child_id < num_slots_) {
        bool left_greater = slots_[right_child_id].priority_ < slots_[left_child_id].priority_;

        if (left_greater && slots_[slot_id].priority_ < slots_[left_child_id].priority_) {
            replace_id = left_child_id;
        }
        else if (!left_greater && slots_[slot_id].priority_ < slots_[right_child_id].priority_) {
            replace_id = right_child_id;
        }
    }
    else if (left_child_id < num_slots_) {
        if (slots_[slot_id].priority_ < slots_[left_child_id].priority_) {
            replace_id = left_child_id;
        }
    }

    if (replace_id != slot_id) {
        Swap(slot_id, replace_id);
        ShuffleDown(replace_id);
    }
}

} // namespace ren

} // namespace lamure

