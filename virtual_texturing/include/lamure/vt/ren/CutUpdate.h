#ifndef LAMURE_CUTUPDATE_H
#define LAMURE_CUTUPDATE_H

#include <lamure/vt/ren/Cut.h>
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

    Cut *start_reading_cut();
    void stop_reading_cut();

    void feedback(uint32_t *buf);

  private:
    std::thread _worker;
    std::mutex _dispatch_lock;
    std::condition_variable _cv;
    std::atomic<bool> _new_feedback;

    TileAtlas<priority_type> *_atlas;
    VTContext *_context;

    Cut _cut;

    uint32_t *_feedback_buffer;

    std::atomic<bool> _should_stop;

    void run();
    void dispatch();
    bool check_children_in_cut(id_type tile_id, const std::set<id_type> &cut);
    void collapse_id(id_type tile_id, std::set<id_type> &cut_new);
    bool split_id(id_type tile_id, std::set<id_type> &cut_new);
    bool memory_available_for_split();
    size_t get_free_mem_slot_index();
    bool try_add_to_indexed_memory(id_type tile_id, uint8_t *tile_ptr);
    void remove_from_indexed_memory(id_type tile_id);
    void keep_id(id_type tile_id, std::set<id_type> &cut_new);

    size_t get_mem_slot_for_id(id_type tile_id);
};
}

#endif // LAMURE_CUTUPDATE_H
