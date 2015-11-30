// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_CUT_UPDATE_QUEUE_H_
#define REN_CUT_UPDATE_QUEUE_H_

#include <mutex>
#include <queue>
#include <lamure/utils.h>

namespace lamure {
namespace ren {

class CutUpdateQueue
{
public:

    enum Task
    {
        CUT_MASTER_TASK,
        CUT_ANALYSIS_TASK,
        CUT_UPDATE_TASK,
        CUT_INVALID_TASK
    };

    struct Job
    {
        explicit Job(
            Task task,
            const view_t view_id,
            const model_t model_id)
            : task_(task),
            view_id_(view_id),
            model_id_(model_id) {};

        explicit Job()
            : task_(Task::CUT_INVALID_TASK),
            view_id_(invalid_view_t),
            model_id_(invalid_model_t) {};

        Task            task_;
        view_t          view_id_;
        model_t         model_id_;
    };

                        CutUpdateQueue();
    virtual             ~CutUpdateQueue();

    void                push_job(const Job& job);
    const Job           pop_frontJob();

    const size_t        NumJobs();


private:
    /* data */

    std::queue<Job>     job_queue_;
    std::mutex          mutex_;

};


} } // namespace lamure


#endif // REN_CUT_UPDATE_QUEUE_H_
