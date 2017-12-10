#ifndef LAMURE_CUTUPDATE_H
#define LAMURE_CUTUPDATE_H

#include <lamure/vt/QuadTree.h>
#include <lamure/vt/common.h>
#include <lamure/vt/ooc/TileAtlas.h>

namespace vt
{
class CutUpdate
{
  public:
    explicit CutUpdate(VTContext *context);
    ~CutUpdate();

    void start();
    void stop();
    void feedback(uint32_t *buf);

  private:
    std::thread _worker;
    std::mutex _dispatch_lock;
    std::condition_variable _cv;
    std::atomic<bool> _new_feedback;
    TileAtlas<priority_type> *_atlas;
    VTContext *_context;

    std::set<id_type> _cut;

    uint8_t *_buf_idx;
    size_t _size_buf_idx;

    id_type *_mem_slots;
    std::queue<size_t> _mem_slots_free;
    std::queue<std::pair<size_t, uint8_t *>> _mem_slots_cut;

    uint32_t *_feedback_buffer;
    size_t _size_feedback;

    std::atomic<bool> _should_stop;

    void run();
    void dispatch();
    bool check_children_in_cut(id_type tile_id, std::set<id_type> &cut);
    void collapse_id(id_type);
    void split_id(id_type tile_id, size_t size_feedback);
    bool memory_available_for_split();
    size_t get_free_mem_slot();
};
}

#endif // LAMURE_CUTUPDATE_H
