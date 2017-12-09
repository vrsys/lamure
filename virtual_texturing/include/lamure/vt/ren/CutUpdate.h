//
// Created by tihi6213 on 12/5/17.
//

#ifndef LAMURE_CUTUPDATE_H
#define LAMURE_CUTUPDATE_H

#include <lamure/vt/common.h>
#include <lamure/vt/QuadTree.h>

class CutUpdate
{
  public:
    CutUpdate();
    ~CutUpdate();

    void start();
    void stop();
    void feedback(std::vector<uint32_t> &buf);

  private:
    uint32_t max_threshold;
    uint32_t min_threshold;

    std::thread _worker;
    std::mutex _dispatch_lock;
    std::condition_variable _cv;

    std::set<uint32_t> _cut;

    std::atomic<bool> _should_stop;

    void run();
    void dispatch();
    bool check_siblings_in_cut(uint32_t _tile_id, std::set<uint32_t> &_cut);
    uint32_t calculate_error_id(uint32_t _tile_id);
    void collapse_id(unsigned int id);
    bool memory_available_for_split();
};

#endif // LAMURE_CUTUPDATE_H
