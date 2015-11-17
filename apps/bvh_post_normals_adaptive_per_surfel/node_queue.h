
#ifndef NODE_QUEUE_H_INCLUDED
#define NODE_QUEUE_H_INCLUDED

#include <lamure/types.h>
#include <thread>
#include <mutex>
#include <queue>
#include <lamure/ren/semaphore.h>

class node_queue_t {
public:

    struct job_t {
        lamure::node_t node_id_;
        int job_id_;

        job_t()
        : job_id_(-1), node_id_(lamure::invalid_node_t) {

        }

        job_t(int job_id, lamure::node_t node_id)
        : job_id_(job_id), node_id_(node_id) {

        }
    };

    node_queue_t();
    ~node_queue_t();

    void PushJob(const job_t& job);
    const job_t PopJob();

    void Wait();
    void Relaunch();
    const bool IsShutdown();
    const unsigned int NumJobs();

private:
    std::queue<job_t> queue_;
    std::mutex mutex_;
    lamure::ren::Semaphore semaphore_;
    bool is_shutdown_;

};




#endif

