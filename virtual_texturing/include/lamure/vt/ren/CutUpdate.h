//
// Created by tihi6213 on 12/5/17.
//

#ifndef LAMURE_CUTUPDATE_H
#define LAMURE_CUTUPDATE_H

#include <lamure/vt/common.h>

class CutUpdate
{
  public:
    CutUpdate();
    ~CutUpdate() = default;

    void start();
    void stop();
    void feedback(std::vector<uint32_t> buf);

  private:
    std::thread _worker;
    std::mutex _dispatch_lock;
    std::condition_variable _cv;

    std::set<uint32_t> _cut;

    std::atomic<bool> _should_stop;

    void run();
    void dispatch();
};

#endif // LAMURE_CUTUPDATE_H
