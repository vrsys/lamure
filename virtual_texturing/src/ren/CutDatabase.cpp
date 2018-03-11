#include <lamure/vt/ren/CutDatabase.h>
namespace vt
{
CutDatabase::CutDatabase(mem_slots_type *front, mem_slots_type *back) : DoubleBuffer<mem_slots_type>(front, back)
{
    VTConfig *config = &VTConfig::get_instance();

    _size_mem_x = config->get_phys_tex_tile_width();
    _size_mem_y = config->get_phys_tex_tile_width();
    _size_feedback = _size_mem_x * _size_mem_y * config->get_phys_tex_layers();

    for(size_t i = 0; i < _size_feedback; i++)
    {
        _front->emplace_back(mem_slot_type{i, UINT64_MAX, nullptr, false, false});
        _back->emplace_back(mem_slot_type{i, UINT64_MAX, nullptr, false, false});
    }

    _cut_map = cut_map_type();
    _tile_provider = new ooc::TileProvider();
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
    for(auto mem_iter = _back->begin(); mem_iter != _back->end(); mem_iter++) // NOLINT
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
uint32_t CutDatabase::register_dataset(const std::string &file_name)
{
    for(dataset_map_entry_type dataset : _dataset_map)
    {
        if(dataset.second == file_name)
        {
            return dataset.first;
        }
    }

    uint32_t id = (uint32_t)_dataset_map.size();
    _dataset_map.insert(dataset_map_entry_type(id, file_name));

    return id;
}
uint16_t CutDatabase::register_view()
{
    uint16_t id = (uint16_t)_view_set.size();
    _view_set.emplace(id);
    return id;
}
uint16_t CutDatabase::register_context()
{
    uint16_t id = (uint16_t)_context_set.size();
    _context_set.emplace(id);
    return id;
}
uint64_t CutDatabase::register_cut(uint32_t dataset_id, uint16_t view_id, uint16_t context_id)
{
    if(_dataset_map.find(dataset_id) == _dataset_map.end())
    {
        throw std::runtime_error("Requested dataset id not registered");
    }

    if(_view_set.find(view_id) == _view_set.end())
    {
        throw std::runtime_error("Requested view id not registered");
    }

    if(_context_set.find(context_id) == _context_set.end())
    {
        throw std::runtime_error("Requested context id not registered");
    }

    Cut *cut = &Cut::init_cut(_tile_provider->addResource(_dataset_map[dataset_id].c_str()));

    uint64_t id = (((uint64_t)dataset_id) << 32) | ((uint64_t)view_id << 16) | ((uint64_t)view_id);

    _cut_map.insert(cut_map_entry_type(id, cut));

    return id;
}

ooc::TileProvider *CutDatabase::get_tile_provider() const { return _tile_provider; }
}