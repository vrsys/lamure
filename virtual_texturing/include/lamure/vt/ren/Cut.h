//
// Created by towe2387 on 12/10/17.
//

#ifndef LAMURE_CUT_H
#define LAMURE_CUT_H

#include <lamure/vt/common.h>
#include <lamure/vt/ooc/TileAtlas.h>
#include <lamure/vt/ren/DoubleBuffer.h>
namespace vt
{
class CutDatabase;
class CutState
{
  public:
    CutState(uint32_t _size_index_buffer);
    ~CutState();

    uint8_t *get_index();
    cut_type &get_cut();
    mem_slots_index_type &get_mem_slots_updated();
    mem_slots_index_type &get_mem_slots_locked();
    void accept(CutState &cut_state);

  private:
    uint32_t _size_index_buffer;

    uint8_t *_index;
    cut_type _cut;
    mem_slots_index_type _mem_slots_updated;
    mem_slots_index_type _mem_slots_locked;
};

class Cut : public DoubleBuffer<CutState>
{
  public:
    static Cut& init_cut(const std::string &file_name);
    ~Cut() override{};

    TileAtlas<priority_type> *get_atlas() const;
    uint16_t get_max_depth() const;
    uint32_t get_size_index_texture() const;

    bool is_drawn() const;
    void set_drawn(bool _drawn);

    static uint32_t get_dataset_id(uint64_t cut_id);
    static uint16_t get_view_id(uint64_t cut_id);
    static uint16_t get_context_id(uint64_t cut_id);

  protected:
    void deliver() override;

  private:
    Cut(TileAtlas<priority_type> *atlas, uint16_t max_depth, uint32_t size_index_texture, CutState *front, CutState *back);

    static uint16_t identify_max_depth(const std::string &file_name);
    static uint32_t identify_size_index_texture(uint64_t max_depth) { return (uint32_t)std::pow(2, max_depth); }

    TileAtlas<priority_type> *_atlas;
    uint16_t _max_depth;
    uint32_t _size_index_texture;

    bool _drawn;
};
}

#endif // LAMURE_CUT_H
