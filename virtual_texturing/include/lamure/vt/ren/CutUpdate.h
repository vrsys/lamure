#ifndef LAMURE_CUTUPDATE_H
#define LAMURE_CUTUPDATE_H

#include <lamure/vt/QuadTree.h>
#include <lamure/vt/common.h>

namespace vt
{
class CutUpdate
{
public:
    CutUpdate();
    ~CutUpdate();

    void start();
    void stop();
    void feedback(uint32_t *buf);

private:
    uint32_t max_threshold;
    uint32_t min_threshold;

    std::thread _worker;
    std::mutex _dispatch_lock;
    std::condition_variable _cv;
    std::atomic<bool> _new_feedback;

    std::set<uint32_t> _cut;
    uint32_t *_feedback_buffer;

    std::atomic<bool> _should_stop;

    void run();
    void dispatch();
    bool check_siblings_in_cut(uint32_t _tile_id, std::set<uint32_t> &_cut);
    uint32_t interpret_feedback(uint32_t _tile_id);
    void collapse_id(unsigned int id);
    bool memory_available_for_split();
};
}

#endif // LAMURE_CUTUPDATE_H
