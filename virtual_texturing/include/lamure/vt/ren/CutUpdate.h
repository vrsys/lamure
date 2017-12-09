#ifndef LAMURE_CUTUPDATE_H
#define LAMURE_CUTUPDATE_H

#include <lamure/vt/common.h>

class CutUpdate
{
  public:
    CutUpdate();
    ~CutUpdate();

    void start();
    void stop();
    void feedback(uint32_t *buf);

  private:
    std::thread _worker;
    std::mutex _dispatch_lock;
    std::condition_variable _cv;
    std::atomic<bool> _new_feedback;

    std::set<uint32_t> _cut;
    uint32_t *_feedback_buffer;

    std::atomic<bool> _should_stop;

    void run();
    void dispatch();
};

#endif // LAMURE_CUTUPDATE_H
