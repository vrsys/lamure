#include <lamure/vt/ren/CutDatabase.h>
namespace vt
{
CutDatabase::CutDatabase(mem_slots_type *front, mem_slots_type *back) : DoubleBuffer<mem_slots_type>(front, back)
{
    VTConfig * config = &VTConfig::get_instance();

    _size_index = config->get_size_index_texture() * config->get_size_index_texture() * 4;
    _size_mem_x = config->get_phys_tex_tile_width();
    _size_mem_y = config->get_phys_tex_tile_width();
    _size_feedback = _size_mem_x * _size_mem_y * config->get_phys_tex_layers();

    for(size_t i = 0; i < _size_feedback; i++)
    {
        _front->emplace_back(mem_slot_type{i, UINT64_MAX, nullptr, false, false});
        _back->emplace_back(mem_slot_type{i, UINT64_MAX, nullptr, false, false});
    }

    // TODO
    CutState *front_state = new CutState(this);
    CutState *back_state = new CutState(this);

    Cut *cut = new Cut(this, front_state, back_state);

    _cut_map = cut_map_type();
    _cut_map[0] = cut;
}
size_t CutDatabase::get_available_memory()
{
    size_t available_memory = _back->size();
    for(cut_map_entry_type cut_entry : _cut_map)
    {
        available_memory -= cut_entry.second->get_back()->get_mem_slots_locked().size();
    }

    return available_memory;
}
mem_slot_type *CutDatabase::get_free_mem_slot()
{
    for(auto mem_iter = _back->begin(); mem_iter != _back->end(); mem_iter++)
    {
        if(!(*mem_iter).locked)
        {
            return &(*mem_iter);
        }
    }

    throw std::runtime_error("out of mem slots");
}
void CutDatabase::deliver() { _front = new mem_slots_type((*_back)); }
Cut *CutDatabase::start_writing_cut(uint64_t cut_id)
{
    Cut *requested_cut = _cut_map[cut_id];
    requested_cut->start_writing();
    return requested_cut;
}
void CutDatabase::stop_writing_cut(uint64_t cut_id) { _cut_map[cut_id]->stop_writing(); }
Cut *CutDatabase::start_reading_cut(uint64_t cut_id)
{
    Cut *requested_cut = _cut_map[cut_id];
    requested_cut->start_reading();
    return requested_cut;
}
void CutDatabase::stop_reading_cut(uint64_t cut_id) { _cut_map[cut_id]->stop_reading(); }
cut_map_type *CutDatabase::get_cut_map() { return &_cut_map; }
}