// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/vt/ren/CutDatabase.h>
namespace vt
{
CutDatabase::CutDatabase() : _context_sync_map()
{
    VTConfig* config = &VTConfig::get_instance();

    _size_mem_x = config->get_phys_tex_tile_width();
    _size_mem_y = config->get_phys_tex_tile_width();
    _size_mem_interleaved = _size_mem_x * _size_mem_y * config->get_phys_tex_layers();

    _cut_map = cut_map_type();
    _tile_provider = new ooc::TileProvider();
}
CutDatabase::~CutDatabase()
{
    _tile_provider->stop();

    delete _tile_provider;

    for(uint16_t context : _context_set)
    {
        delete _context_sync_map[context];
        delete _context_state_map[context];
    }

    for(auto cut_entry : _cut_map)
    {
        delete cut_entry.second;
    }
}
void CutDatabase::warm_up_cache()
{
    _tile_provider->start((size_t)VTConfig::get_instance().get_size_ram_cache() * 1024 * 1024);

    for(auto cut_entry : _cut_map)
    {
        auto cut = cut_entry.second;
        auto atlas = cut->get_atlas();

        if(atlas->getDepth() < 1)
        {
            return;
        }

        for(uint32_t depth = 0; depth < atlas->getDepth() && depth < 5; depth++)
        {
            auto first = QuadTree::get_first_node_id_of_depth(depth);
            auto length = QuadTree::get_length_of_depth(depth);

            for(uint64_t tile_id = first; tile_id < first + length; tile_id++)
            {
                _tile_provider->getTile(atlas, tile_id, 0.f, Cut::get_context_id(cut->get_id()));
            }
        }
    }

    _tile_provider->wait(std::chrono::milliseconds(10000));
}
size_t CutDatabase::get_available_memory(uint16_t context_id)
{
    size_t available_memory = _context_state_map[context_id]->get_back()->size();
    for(cut_map_entry_type cut_entry : _cut_map)
    {
        if(Cut::get_context_id(cut_entry.first) != context_id)
        {
            continue;
        }
        available_memory -= cut_entry.second->get_back()->get_mem_slots_locked().size();
    }

    return available_memory;
}
mem_slot_type* CutDatabase::get_free_mem_slot(uint16_t context_id)
{
    for(auto mem_iter = _context_state_map[context_id]->get_back()->begin(); mem_iter != _context_state_map[context_id]->get_back()->end(); mem_iter++) // NOLINT
    {
        if(!(*mem_iter).locked)
        {
            return &(*mem_iter);
        }
    }

    throw std::runtime_error("out of mem slots");
}
mem_slot_type* CutDatabase::write_mem_slot_at(size_t position, uint16_t context_id)
{
    std::unique_lock<std::mutex> lk(_context_sync_map[context_id]->_write_lock);

    if(position >= _size_mem_interleaved)
    {
        throw std::runtime_error("Write request to interleaved memory position: " + std::to_string(position) + ", interleaved memory size is: " + std::to_string(_size_mem_interleaved));
    }

    if(!_context_sync_map[context_id]->_is_written.load())
    {
        throw std::runtime_error("Unsanctioned write request to interleaved memory position: " + std::to_string(position));
    }

    return &_context_state_map[context_id]->get_back()->at(position);
}
mem_slot_type* CutDatabase::read_mem_slot_at(size_t position, uint16_t context_id)
{
    // std::cout << "read_mem_slot_at" << std::endl;

    std::unique_lock<std::mutex> lk(_context_sync_map[context_id]->_read_lock);

    if(position >= _size_mem_interleaved)
    {
        throw std::runtime_error("Read request to position: " + std::to_string(position) + ", interleaved memory size is: " + std::to_string(_size_mem_interleaved));
    }

    if(!_context_sync_map[context_id]->_is_read.load())
    {
        throw std::runtime_error("Unsanctioned read request to interleaved memory position: " + std::to_string(position));
    }

    return &_context_state_map[context_id]->get_front()->at(position);
}
Cut* CutDatabase::start_writing_cut(uint64_t cut_id)
{
    // std::cout << "start_writing_cut" << std::endl;

    uint16_t context_id = Cut::get_context_id(cut_id);

    std::unique_lock<std::mutex> lk(_context_sync_map[context_id]->_write_lock);

    if(_context_sync_map[context_id]->_is_written.load())
    {
        throw std::runtime_error("Memory write access corruption");
    }
    _context_sync_map[context_id]->_is_written.store(true);

    _context_state_map[context_id]->start_writing();
    Cut* requested_cut = _cut_map[cut_id];
    requested_cut->start_writing();

    return requested_cut;
}
void CutDatabase::stop_writing_cut(uint64_t cut_id)
{
    // std::cout << "stop_writing_cut" << std::endl;

    uint16_t context_id = Cut::get_context_id(cut_id);

    std::unique_lock<std::mutex> lk(_context_sync_map[context_id]->_write_lock);

    _cut_map[cut_id]->stop_writing();
    _context_state_map[context_id]->stop_writing();

    if(!_context_sync_map[context_id]->_is_written.load())
    {
        throw std::runtime_error("Memory write access corruption");
    }
    _context_sync_map[context_id]->_is_written.store(false);

    _context_sync_map[context_id]->_read_write_cv.notify_one();
}
Cut* CutDatabase::start_reading_cut(uint64_t cut_id)
{
    // std::cout << "start_reading_cut" << std::endl;

    uint16_t context_id = Cut::get_context_id(cut_id);

    std::unique_lock<std::mutex> lk(_context_sync_map[context_id]->_read_lock);

    if(_context_sync_map[context_id]->_is_read.load())
    {
        throw std::runtime_error("Memory read access corruption");
    }

    std::unique_lock<std::mutex> cv_lk(_context_sync_map[context_id]->_read_write_lock);
    _context_sync_map[context_id]->_read_write_cv.wait(cv_lk, [&]() -> bool { return !_context_sync_map[context_id]->_is_written.load(); });

    _context_sync_map[context_id]->_is_read.store(true);

    // std::cout << "start_reading_cut: is being read" << std::endl;

    _context_state_map[context_id]->start_reading();
    Cut* requested_cut = _cut_map[cut_id];
    requested_cut->start_reading();

    return requested_cut;
}
void CutDatabase::stop_reading_cut(uint64_t cut_id)
{
    // std::cout << "stop_reading_cut" << std::endl;

    uint16_t context_id = Cut::get_context_id(cut_id);

    std::unique_lock<std::mutex> lk(_context_sync_map[context_id]->_read_lock);

    _context_state_map[context_id]->stop_reading();
    _cut_map[cut_id]->stop_reading();

    if(!_context_sync_map[context_id]->_is_read.load())
    {
        throw std::runtime_error("Memory read access corruption");
    }
    _context_sync_map[context_id]->_is_read.store(false);

    // std::cout << "stop_reading_cut: is not being read any longer" << std::endl;
}
cut_map_type* CutDatabase::get_cut_map() { return &_cut_map; }
uint32_t CutDatabase::register_dataset(const std::string& file_name)
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

    _context_sync_map.insert({id, new SyncStructure()});

    mem_slots_type* front = new mem_slots_type();
    mem_slots_type* back = new mem_slots_type();

    for(size_t i = 0; i < _size_mem_interleaved; i++)
    {
        front->emplace_back(mem_slot_type{i, UINT64_MAX, nullptr, false, false});
        back->emplace_back(mem_slot_type{i, UINT64_MAX, nullptr, false, false});
    }

    _context_state_map.insert({id, new StateStructure(front, back)});

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

    uint64_t id = ((uint64_t)dataset_id) << 32 | ((uint64_t)view_id << 16) | ((uint64_t)context_id);

    Cut* cut = &Cut::init_cut(id, _tile_provider->loadResource(_dataset_map[dataset_id].c_str()));

    if(cut->get_atlas()->getDepth() < 1)
    {
        std::cout << "tree is too flat" << std::endl;
        exit(1);
    }

    _cut_map.insert(cut_map_entry_type(id, cut));

    return id;
}

ooc::TileProvider* CutDatabase::get_tile_provider() const { return _tile_provider; }
view_set_type* CutDatabase::get_view_set() { return &_view_set; }
context_set_type* CutDatabase::get_context_set() { return &_context_set; }
} // namespace vt