#ifndef LAMURE_CUTDATABASE_H
#define LAMURE_CUTDATABASE_H

#include <lamure/vt/common.h>
#include <lamure/vt/ren/Cut.h>
#include <lamure/vt/VTConfig.h>
namespace vt
{
class VTContext;
class CutDatabase : public DoubleBuffer<mem_slots_type>
{
public:
    static CutDatabase &get_instance()
    {
        // TODO
        mem_slots_type *front = new mem_slots_type();
        mem_slots_type *back = new mem_slots_type();

        static CutDatabase instance(front, back);
        return instance;
    }
    CutDatabase(CutDatabase const &) = delete;
    void operator=(CutDatabase const &) = delete;

  public:
    ~CutDatabase() override {}

    size_t get_available_memory();
    mem_slot_type *get_free_mem_slot();

    size_t get_size_index() const { return _size_index; }
    size_t get_size_mem_x() const { return _size_mem_x; }
    size_t get_size_mem_y() const { return _size_mem_y; }
    size_t get_size_feedback() const { return _size_feedback; }

    Cut *start_writing_cut(uint64_t cut_id);
    void stop_writing_cut(uint64_t cut_id);

    Cut *start_reading_cut(uint64_t cut_id);
    void stop_reading_cut(uint64_t cut_id);

    cut_map_type *get_cut_map();

  protected:
    explicit CutDatabase(mem_slots_type *front, mem_slots_type *back);

    void deliver() override;

  private:
    size_t _size_index;
    size_t _size_mem_x;
    size_t _size_mem_y;
    size_t _size_feedback;

    cut_map_type _cut_map;
};
}

#endif // LAMURE_CUTDATABASE_H
