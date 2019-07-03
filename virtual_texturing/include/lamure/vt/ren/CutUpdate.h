// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_CUTUPDATE_H
#define LAMURE_CUTUPDATE_H

#include <lamure/vt/QuadTree.h>
#include <lamure/vt/VTConfig.h>
#include <lamure/vt/common.h>
#include <lamure/vt/ren/Cut.h>

namespace vt
{
class VT_DLL CutUpdate
{
  public:
    friend class ContextFeedback;
    static CutUpdate& get_instance()
    {
        static CutUpdate instance;
        return instance;
    }
    CutUpdate(CutUpdate const&) = delete;
    void operator=(CutUpdate const&) = delete;

    ~CutUpdate();

    void start();
    void stop();

    ContextFeedback* get_context_feedback(uint16_t context_id);

    bool can_accept_feedback(uint32_t context_id);
    void feedback(uint32_t context_id, int32_t* buf_lod, uint32_t* buf_count);
    const float& get_dispatch_time() const;

    void toggle_freeze_dispatch();

  private:
    CutUpdate();

    context_feedback_map_type _context_feedbacks;
    cut_decision_map_type _cut_decisions;

    VTConfig* _config;
    CutDatabase* _cut_db;

    float _dispatch_time;
    uint32_t _precomputed_split_budget_throughput;

    std::atomic<bool> _should_stop;
    std::atomic<bool> _freeze_dispatch;

    void run(ContextFeedback* _context_feedback);
    void dispatch_context(uint16_t context_id);

    bool collapse_to_id(Cut* cut, id_type tile_id, uint16_t context_id);
    bool split_id(Cut *cut, prioritized_tile tile, uint16_t context_id);
    bool keep_id(Cut* cut, id_type tile_id, uint16_t context_id);

    bool add_to_indexed_memory(Cut* cut, id_type tile_id, uint8_t* tile_ptr, uint16_t context_id);
    mem_slot_type* write_mem_slot_for_id(Cut* cut, id_type tile_id, uint16_t context_id);

    bool check_all_siblings_in_cut(id_type tile_id, const cut_type& cut);
    void remove_from_indexed_memory(Cut* cut, id_type tile_id, uint16_t context_id);
};

class VT_DLL CutDecision
{
  public:
    friend class CutUpdate;

    CutDecision() : collapse_to(), split(), keep() {}
    ~CutDecision() {}

  private:
    id_set_type collapse_to;
    prioritized_tile_set_type split;
    id_set_type keep;
};

class VT_DLL ContextFeedback
{
  public:
    friend class CutUpdate;

    ContextFeedback(uint16_t id, CutUpdate* cut_update) : _feedback_dispatch_lock(), _feedback_cv(), _feedback_new(), _allocated_slot_index()
    {
        _id = id;

        _feedback_new.store(false);

        _feedback_lod_buffer = new int32_t[CutDatabase::get_instance().get_size_mem_interleaved()];
#ifdef RASTERIZATION_COUNT
        _feedback_count_buffer = new uint32_t[CutDatabase::get_instance().get_size_mem_interleaved()];
#endif

        _feedback_worker = std::thread(&CutUpdate::run, cut_update, this);
    }

    ~ContextFeedback()
    {
        _feedback_new.store(false);
        _feedback_cv.notify_one();

        if(_feedback_worker.joinable())
        {
            _feedback_worker.join();
        }

        _allocated_slot_index.clear();

        delete[] _feedback_lod_buffer;
#ifdef RASTERIZATION_COUNT
        delete[] _feedback_count_buffer;
#endif
    }

    std::set<uint32_t>& get_allocated_slot_index() { return _allocated_slot_index; }
    uint32_t get_compact_position(uint32_t position)
    {
        auto iter = std::find(_allocated_slot_index.begin(), _allocated_slot_index.end(), position);

        if(iter != _allocated_slot_index.end())
        {
            return (uint32_t)std::distance(_allocated_slot_index.begin(), iter);
        }

        return 0;
    }

  private:
    uint16_t _id;

    std::atomic<bool> _feedback_new;
    std::mutex _feedback_dispatch_lock;
    std::condition_variable _feedback_cv;
    std::thread _feedback_worker;
    std::set<uint32_t> _allocated_slot_index;

    int32_t* _feedback_lod_buffer;
#ifdef RASTERIZATION_COUNT
    uint32_t* _feedback_count_buffer;
#endif
};
} // namespace vt

#endif // LAMURE_CUTUPDATE_H
