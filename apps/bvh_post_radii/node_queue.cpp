// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "node_queue.h"


node_queue_t::
node_queue_t()
: is_shutdown_(false) {
    semaphore_.set_min_signal_count(1);
    semaphore_.set_max_signal_count(std::numeric_limits<float>::max());
}

node_queue_t::
~node_queue_t() {

}

void node_queue_t::
wait() {
    semaphore_.wait();
}

const bool node_queue_t::
is_shutdown() {
    return is_shutdown_;
}

void node_queue_t::
relaunch() {
    is_shutdown_ = false;
}

const unsigned int node_queue_t::
Numjobs() {
    return (unsigned int)queue_.size();
}

void node_queue_t::
push_job(const node_queue_t::job_t& job) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(job);
    semaphore_.Signal(1);

}

const node_queue_t::job_t node_queue_t::
pop_job() {
    std::lock_guard<std::mutex> lock(mutex_);

    job_t job;

    if (!queue_.empty()) {
        job = queue_.front();
        queue_.pop();
    }

    if (queue_.empty()) {
        is_shutdown_ = true;
        semaphore_.shutdown();
    }

    return job;
}


