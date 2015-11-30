// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_CACHE_QUEUE_H_
#define REN_CACHE_QUEUE_H_

#include <mutex>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <deque>

#include <lamure/ren/platform.h>
#include <lamure/utils.h>

namespace lamure {
namespace ren {
class RENDERING_DLL CacheQueue
{
public:

    enum UpdateMode
    {
        UPDATE_ALWAYS,
        UPDATE_INCREMENT_ONLY,
        UPDATE_DECREMENT_ONLY,
        UPDATE_NEVER
    };

    enum QueryResult
    {
        NOT_INDEXED,
        INDEXED_AS_WAITING,
        INDEXED_AS_LOADING
    };

    enum AbortResult
    {
        ABORT_SUCCESS,
        ABORT_FAILED
    };

    struct Job
    {
        explicit Job(
            const model_t model_id,
            const node_t node_id,
            const slot_t slot_id,
            const int32_t priority,
            char* const slot_mem)
            : model_id_(model_id),
            node_id_(node_id),
            slot_id_(slot_id),
            priority_(priority),
            slot_mem_(slot_mem) {};

        explicit Job()
            : model_id_(invalid_model_t),
            node_id_(invalid_node_t),
            slot_id_(invalid_slot_t),
            priority_(0),
            slot_mem_(nullptr) {};

        model_t         model_id_;
        node_t          node_id_;
        slot_t          slot_id_;
        int32_t         priority_;
        char*           slot_mem_;
    };

                        CacheQueue();
    virtual             ~CacheQueue();

    bool                push_job(const Job& job);
    const Job           TopJob();
    void                pop_job(const Job& job);
    void                UpdateJob(const model_t model_id, const node_t node_id, int32_t priority);
    const AbortResult   AbortJob(const Job& job);

    const size_t        NumJobs();
    void                Initialize(const UpdateMode mode, const model_t num_models);
    const QueryResult   IsNodeIndexed(const model_t model_id, const node_t node_id);

private:
    void                Swap(const size_t slot_id_0, const size_t slot_id_1);
    void                ShuffleUp(const size_t slot_id);
    void                ShuffleDown(const size_t slot_id);

    size_t              num_slots_;
    model_t             num_models_;
    std::mutex          mutex_;
    UpdateMode          mode_;
    bool                initialized_;

    std::vector<Job>    slots_;

    //mapping (model, node) to slot
    std::vector<std::unordered_map<node_t, slot_t>> requested_set_;
    std::vector<std::unordered_set<node_t>> pending_set_;
};


} } // namespace lamure


#endif // REN_CACHE_QUEUE_H_
