// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_CUTDATABASE_H
#define LAMURE_CUTDATABASE_H

#include <lamure/vt/QuadTree.h>
#include <lamure/vt/VTConfig.h>
#include <lamure/vt/common.h>
#include <lamure/vt/ooc/TileProvider.h>
#include <lamure/vt/ren/Cut.h>
namespace vt
{
class VTContext;
class SyncStructure
{
  public:
    SyncStructure() : _read_lock(), _write_lock(), _read_write_lock(), _is_written(false), _is_read(false), _read_write_cv() {}
    ~SyncStructure() = default;

    std::atomic<bool> _is_written, _is_read;
    std::mutex _read_lock, _write_lock, _read_write_lock;
    std::condition_variable _read_write_cv;
};
class StateStructure : public DoubleBuffer<mem_slots_type>
{
  public:
    StateStructure(mem_slots_type* front, mem_slots_type* back) : DoubleBuffer<mem_slots_type>(front, back) {}
    ~StateStructure() = default;

    void deliver() override { _front->assign(_back->begin(), _back->end()); }
};
class CutDatabase
{
  public:
    static CutDatabase& get_instance()
    {
        static CutDatabase instance;
        return instance;
    }
    CutDatabase(CutDatabase const&) = delete;
    void operator=(CutDatabase const&) = delete;

    ~CutDatabase();

    void warm_up_cache();

    size_t get_available_memory(uint16_t context_id);
    mem_slot_type* get_free_mem_slot(uint16_t context_id);
    mem_slot_type* write_mem_slot_at(size_t position, uint16_t context_id);
    mem_slot_type* read_mem_slot_at(size_t position, uint16_t context_id);

    size_t get_size_mem_x() const { return _size_mem_x; }
    size_t get_size_mem_y() const { return _size_mem_y; }
    size_t get_size_mem_interleaved() const { return _size_mem_interleaved; }

    uint32_t register_dataset(const std::string& file_name);
    uint16_t register_view();
    uint16_t register_context();
    uint64_t register_cut(uint32_t dataset_id, uint16_t view_id, uint16_t context_id);

    Cut* start_writing_cut(uint64_t cut_id);
    void stop_writing_cut(uint64_t cut_id);

    Cut* start_reading_cut(uint64_t cut_id);
    void stop_reading_cut(uint64_t cut_id);

    cut_map_type* get_cut_map();
    view_set_type* get_view_set();
    context_set_type* get_context_set();

    ooc::TileProvider* get_tile_provider() const;

  protected:
    CutDatabase();

  private:
    size_t _size_mem_x;
    size_t _size_mem_y;
    size_t _size_mem_interleaved;

    dataset_map_type _dataset_map;
    view_set_type _view_set;
    context_set_type _context_set;
    cut_map_type _cut_map;

    ooc::TileProvider* _tile_provider;

    std::map<uint16_t, SyncStructure*> _context_sync_map;
    std::map<uint16_t, StateStructure*> _context_state_map;
};
} // namespace vt

#endif // LAMURE_CUTDATABASE_H
