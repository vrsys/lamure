#ifndef LAMURE_CUTUPDATE_H
#define LAMURE_CUTUPDATE_H

#include <lamure/vt/QuadTree.h>
#include <lamure/vt/common.h>
#include <lamure/vt/ooc/TileAtlas.h>
#include <lamure/vt/ren/Cut.h>

namespace vt
{
typedef std::set<id_type, QuadTree::more_than_by_depth> ordered_quadtree_set_type;

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
    const float &get_dispatch_time() const;

    void set_freeze_dispatch(bool _freeze_dispatch);
    bool get_freeze_dispatch();

private:
    std::thread _worker;
    std::mutex _dispatch_lock;
    std::condition_variable _cv;
    std::atomic<bool> _new_feedback;

    TileAtlas<priority_type> *_atlas;
    VTContext *_context;

    Cut _cut;

    float _dispatch_time;

    uint32_t *_feedback_buffer;

    std::atomic<bool> _should_stop;

    bool _freeze_dispatch = false;

    void run();
    void dispatch();
    bool collapse_to_id(id_type tile_id);
    bool split_id(id_type tile_id);
    bool keep_id(id_type tile_id);
    bool memory_available_for_split();
    bool try_add_to_indexed_memory(id_type tile_id, uint8_t *tile_ptr);
    void remove_from_indexed_memory(id_type tile_id);

    void identify_effective_cut(cut_type &cut);
    void identify_effective_update(cut_type &cut);

    uint8_t count_children_in_cut(id_type tile_id);
    bool check_all_siblings_in_cut(id_type tile_id);
    size_t get_free_mem_slot_index();
    size_t get_mem_slot_for_id(id_type tile_id);
};
}

#endif // LAMURE_CUTUPDATE_H
