
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
Wait() {
    semaphore_.Wait();
}

const bool node_queue_t::
IsShutdown() {
    return is_shutdown_;
}

void node_queue_t::
Relaunch() {
    is_shutdown_ = false;
}

const unsigned int node_queue_t::
NumJobs() {
    return (unsigned int)queue_.size();
}

void node_queue_t::
PushJob(const node_queue_t::job_t& job) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(job);
    semaphore_.Signal(1);

}

const node_queue_t::job_t node_queue_t::
PopJob() {
    std::lock_guard<std::mutex> lock(mutex_);

    job_t job;

    if (!queue_.empty()) {
        job = queue_.front();
        queue_.pop();
    }

    if (queue_.empty()) {
        is_shutdown_ = true;
        semaphore_.Shutdown();
    }

    return job;
}


