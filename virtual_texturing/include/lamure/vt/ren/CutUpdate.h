#ifndef LAMURE_CUTUPDATE_H
#define LAMURE_CUTUPDATE_H

#include <lamure/vt/QuadTree.h>
#include <lamure/vt/common.h>
#include <lamure/vt/ooc/TileAtlas.h>
#include <lamure/vt/DoubleBuffer.h>
#include "VTRenderer.h"

namespace vt
{
class CutUpdate
{
public:
    //CutUpdate() = default;
    explicit CutUpdate(VTContext *context, TileAtlas<priority_type> *atlas);

    ~CutUpdate();

    void start();
    void stop();
    void feedback(uint32_t *buf);

    void set_renderer(VTRenderer *renderer);

    uint8_t *start_reading_idx(){
        return _idx_buffer.startReading();
    }

    void stop_reading_idx(){
        _idx_buffer.stopReading();
    }

private:
    uint32_t max_threshold;
    uint32_t min_threshold;

    std::thread _worker;
    std::mutex _dispatch_lock;
    std::condition_variable _cv;
    std::atomic<bool> _new_feedback;
    TileAtlas<priority_type> *_atlas;
    VTRenderer *_renderer;
    VTContext *_context;
    DoubleBuffer _idx_buffer;

    std::set<uint32_t> _cut;
    uint32_t *_feedback_buffer;

    std::atomic<bool> _should_stop;

    id_type *_slots;

    void run();
    void dispatch();
    bool check_siblings_in_cut(uint32_t _tile_id, std::set<uint32_t> &_cut);
    uint32_t interpret_feedback(uint32_t _tile_id);
    void collapse_id(unsigned int id);
    bool memory_available_for_split();
    size_t get_free_slot();
};
}

#endif // LAMURE_CUTUPDATE_H
