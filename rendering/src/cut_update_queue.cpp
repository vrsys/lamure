// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/cut_update_queue.h>


namespace lamure
{

namespace ren
{

CutUpdateQueue::
CutUpdateQueue() {

}

CutUpdateQueue::
~CutUpdateQueue() {

}

void CutUpdateQueue::
PushJob(const Job& job) {
    std::lock_guard<std::mutex> lock(mutex_);
    job_queue_.push(job);
}

const CutUpdateQueue::Job CutUpdateQueue::
PopFrontJob() {
    std::lock_guard<std::mutex> lock(mutex_);

    Job job;

    if (!job_queue_.empty()) {
        job = job_queue_.front();
        job_queue_.pop();
    }

    return job;
}

const size_t CutUpdateQueue::
NumJobs() {
    std::lock_guard<std::mutex> lock(mutex_);
    return job_queue_.size();
}

} // namespace ren

} // namespace lamure

