#ifndef LAMURE_CUTUPDATE_H
#define LAMURE_CUTUPDATE_H

#include <lamure/vt/QuadTree.h>
#include <lamure/vt/common.h>
#include <lamure/vt/VTConfig.h>
#include <lamure/vt/ooc/TileAtlas.h>
#include <lamure/vt/ren/Cut.h>

namespace vt
{
typedef std::set<id_type> id_set_type;

class CutUpdate
{
  public:
    explicit CutUpdate();
    ~CutUpdate();

    void start();
    void stop();

    void feedback(uint32_t *buf);
    const float &get_dispatch_time() const;

    void set_freeze_dispatch(bool _freeze_dispatch);
    bool get_freeze_dispatch();

private:
    std::thread _worker;
    std::mutex _dispatch_lock;
    std::condition_variable _cv;
    std::atomic<bool> _new_feedback;

    VTConfig *_config;
    CutDatabase *_cut_db;

    float _dispatch_time;

    uint32_t *_feedback_buffer;

    std::atomic<bool> _should_stop;

    bool _freeze_dispatch = false;

    void run();
    void dispatch();

    bool collapse_to_id(Cut *cut, id_type tile_id);
    bool split_id(Cut *cut, id_type tile_id);
    bool keep_id(Cut *cut, id_type tile_id);

    bool add_to_indexed_memory(Cut *cut, id_type tile_id, uint8_t *tile_ptr);
    mem_slot_type *get_mem_slot_for_id(Cut *cut, id_type tile_id);

    uint8_t count_children_in_cut(id_type tile_id, const cut_type &cut);
    bool check_all_siblings_in_cut(id_type tile_id, const cut_type &cut);
};
}

#endif // LAMURE_CUTUPDATE_H
