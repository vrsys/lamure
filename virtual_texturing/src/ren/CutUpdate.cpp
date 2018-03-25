#include <lamure/vt/ren/CutDatabase.h>
#include <lamure/vt/ren/CutUpdate.h>

namespace vt
{
CutUpdate::CutUpdate() : _dispatch_lock(), _dispatch_time()
{
    _should_stop.store(false);
    _new_feedback.store(false);

    _config = &VTConfig::get_instance();
    _cut_db = &CutDatabase::get_instance();

    _feedback_lod_buffer = new int32_t[_cut_db->get_size_mem_interleaved()];
    _feedback_count_buffer = new uint32_t[_cut_db->get_size_mem_interleaved()];
}

CutUpdate::~CutUpdate() {}

void CutUpdate::start()
{
    _cut_db->get_tile_provider()->start((size_t)VTConfig::get_instance().get_size_ram_cache() * 1024 * 1024);
    _worker = std::thread(&CutUpdate::run, this);
}

void CutUpdate::run()
{
    while(!_should_stop.load())
    {
        std::unique_lock<std::mutex> lk(_dispatch_lock, std::defer_lock);
        if(_new_feedback.load())
        {
            dispatch();
        }
        _new_feedback.store(false);

        while(!_cv.wait_for(lk, std::chrono::milliseconds(100), [this] { return _new_feedback.load(); }))
        {
            if(_should_stop.load())
            {
                break;
            }
        }
    }
}

void CutUpdate::dispatch()
{
    if(_freeze_dispatch)
    {
        return;
    }

    // std::cout << "\ndispatch() BEGIN" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    uint32_t texels_per_tile = _config->get_size_tile() * _config->get_size_tile();
    uint32_t throughput = _config->get_size_physical_update_throughput();
    uint32_t split_budget_throughput = throughput * 1024 * 1024 / texels_per_tile / _config->get_byte_stride() / 4;
    uint32_t split_budget_available = (uint32_t)_cut_db->get_available_memory() / 4;
    uint32_t split_budget = std::min(split_budget_throughput, split_budget_available);

    // std::cout << "split budget: " << split_budget << std::endl;

    uint16_t split_counter = 0;

    for(cut_map_entry_type cut_entry : (*_cut_db->get_cut_map()))
    {
        id_set_type collapse_to;
        id_set_type split;
        id_set_type keep;

        Cut *cut = _cut_db->start_reading_cut(cut_entry.first);

        // std::cout << std::endl;
        // std::cout << "writing cut: " << cut_entry.first << std::endl;
        // std::cout << std::endl;

        if(!cut->is_drawn())
        {
            _cut_db->stop_reading_cut(cut_entry.first);

            cut = _cut_db->start_writing_cut(cut_entry.first);

            _cut_db->get_tile_provider()->getTile(cut->get_atlas(), 0, 100);

            if(!_cut_db->get_tile_provider()->wait(std::chrono::milliseconds(1000)))
            {
                throw std::runtime_error("Root tile not loaded for atlas: " + std::string(cut->get_atlas()->getFileName()));
            }

            ooc::TileCacheSlot *slot = _cut_db->get_tile_provider()->getTile(cut->get_atlas(), 0, 100);

            if(slot == nullptr)
            {
                throw std::runtime_error("Root tile is nullptr for atlas: " + std::string(cut->get_atlas()->getFileName()));
            }

            uint8_t *root_tile = slot->getBuffer();

            add_to_indexed_memory(cut, 0, root_tile);

            cut->set_drawn(true);

            _cut_db->stop_writing_cut(cut_entry.first);

            continue;
        }

        cut_type cut_desired = cut_type(cut->get_front()->get_cut());

        /* DECISION MAKING PASS */

        for(auto iter = cut->get_front()->get_cut().crbegin(); iter != cut->get_front()->get_cut().crend(); iter++)
        {
            id_type tile_id = *iter;
            id_type parent_id = QuadTree::get_parent_id(tile_id);
            mem_slot_type *mem_slot = read_mem_slot_for_id(cut, tile_id);

            uint16_t tile_depth = QuadTree::get_depth_of_node(tile_id);

            if(mem_slot == nullptr)
            {
                std::cerr << "Node " << std::to_string(tile_id) << " not found in memory slots" << std::endl;
                continue;
            }

            if(_feedback_lod_buffer[mem_slot->position] > tile_depth && tile_depth < (cut->get_atlas()->getDepth() - 1) && split_counter < split_budget)
            {
                split_counter++;

                // std::cout << "decision: split " << tile_id << ", " << _feedback_buffer[mem_slot->position] << std::endl;

                cut_desired.erase(tile_id);

                for(uint8_t i = 0; i < 4; i++)
                {
                    cut_desired.insert(QuadTree::get_child_id(tile_id, i));
                }

                split.insert(tile_id);
            }
            else if(_feedback_lod_buffer[mem_slot->position]!=0 && _feedback_lod_buffer[mem_slot->position] < tile_depth && check_all_siblings_in_cut(tile_id, cut->get_back()->get_cut()) && tile_depth > 0)
            {
                // std::cout << "decision: collapse to " << parent_id << ", " << _feedback_buffer[mem_slot->position] << std::endl;

                for(uint8_t i = 0; i < 4; i++)
                {
                    cut_desired.erase(QuadTree::get_child_id(parent_id, i));
                }

                cut_desired.insert(parent_id);

                collapse_to.insert(parent_id);
            }
            else
            {
                // std::cout << "decision: keep " << tile_id << ", " << _feedback_buffer[mem_slot->position] << std::endl;

                keep.insert(tile_id);
            }
        }

        _cut_db->stop_reading_cut(cut_entry.first);

        cut = _cut_db->start_writing_cut(cut_entry.first);

        cut->get_back()->get_cut().swap(cut_desired);

        // std::cout << "collapsing to " << collapse_to.size() << " nodes" << std::endl;
        // std::cout << "splitting " << split.size() << " nodes" << std::endl;
        // std::cout << "keeping " << keep.size() << " nodes" << std::endl;

        /* MEMORY INDEXING PASS */

        for(id_type tile_id : collapse_to)
        {
            // std::cout << "action: collapse to " << tile_id << std::endl;
            if(!collapse_to_id(cut, tile_id))
            {
                cut->get_back()->get_cut().erase(tile_id);

                for(uint8_t i = 0; i < 4; i++)
                {
                    id_type child_id = QuadTree::get_child_id(tile_id, i);

                    cut->get_back()->get_cut().insert(child_id);
                    keep.insert(child_id);
                }
            }
        }

        for(id_type tile_id : split)
        {
            // std::cout << "action: split " << tile_id << std::endl;
            if(!split_id(cut, tile_id))
            {
                for(uint8_t i = 0; i < 4; i++)
                {
                    id_type child_id = QuadTree::get_child_id(tile_id, i);

                    cut->get_back()->get_cut().erase(child_id);
                }

                cut->get_back()->get_cut().insert(tile_id);
                keep.insert(tile_id);
            }
        }

        for(id_type tile_id : keep)
        {
            // std::cout << "action: keep " << tile_id << std::endl;
            keep_id(cut, tile_id);
        }

        _cut_db->stop_writing_cut(cut_entry.first);
    }

    auto end = std::chrono::high_resolution_clock::now();
    _dispatch_time = std::chrono::duration<float, std::milli>(end - start).count();

    // std::cout << "dispatch() END" << std::endl;
}

bool CutUpdate::add_to_indexed_memory(Cut *cut, id_type tile_id, uint8_t *tile_ptr)
{
    mem_slot_type *mem_slot = write_mem_slot_for_id(cut, tile_id);

    if(mem_slot == nullptr)
    {
        mem_slot_type *free_mem_slot = _cut_db->get_free_mem_slot();

        if(free_mem_slot == nullptr)
        {
            throw std::runtime_error("out of mem slots");
        }

        free_mem_slot->tile_id = tile_id;
        free_mem_slot->pointer = tile_ptr;

        mem_slot = free_mem_slot;
    }

    mem_slot->locked = true;
    mem_slot->updated = false;

    cut->get_back()->get_mem_slots_locked()[tile_id] = mem_slot->position;

    uint_fast32_t x_orig, y_orig;
    QuadTree::get_pos_by_id(tile_id, x_orig, y_orig);
    uint16_t tile_depth = QuadTree::get_depth_of_node(tile_id);

    size_t phys_tex_tile_width = _config->get_phys_tex_tile_width();
    size_t tiles_per_tex = phys_tex_tile_width * phys_tex_tile_width;

    uint8_t *ptr = &cut->get_back()->get_index(tile_depth)[(y_orig * QuadTree::get_tiles_per_row(tile_depth) + x_orig) * 4];

    size_t level = mem_slot->position / tiles_per_tex;
    size_t rel_pos = mem_slot->position - level * tiles_per_tex;
    size_t x_tile = rel_pos % phys_tex_tile_width;
    size_t y_tile = rel_pos / phys_tex_tile_width;

    if(ptr[0] != (uint8_t)x_tile || ptr[1] != (uint8_t)y_tile || ptr[2] != (uint8_t)level || ptr[3] != (uint8_t)1)
    {
        mem_slot->updated = true;
        ptr[0] = (uint8_t)x_tile;
        ptr[1] = (uint8_t)y_tile;
        ptr[2] = (uint8_t)level;
        ptr[3] = (uint8_t)1;
    }

    cut->get_back()->get_cut().insert(tile_id);

    if(mem_slot->updated)
    {
        cut->get_back()->get_mem_slots_updated()[tile_id] = mem_slot->position;
    }
    else
    {
        cut->get_back()->get_mem_slots_updated().erase(tile_id);
    };

    return true;
}

bool CutUpdate::collapse_to_id(Cut *cut, id_type tile_id)
{
    ooc::TileCacheSlot *slot = _cut_db->get_tile_provider()->getTile(cut->get_atlas(), tile_id, 100);

    if(slot == nullptr)
    {
        return false;
    }

    uint8_t *tile_ptr = slot->getBuffer();

    for(uint8_t i = 0; i < 4; i++)
    {
        remove_from_indexed_memory(cut, QuadTree::get_child_id(tile_id, i));
    }

    return add_to_indexed_memory(cut, tile_id, tile_ptr);
}
bool CutUpdate::split_id(Cut *cut, id_type tile_id)
{
    bool all_children_available = true;

    for(size_t n = 0; n < 4; n++)
    {
        id_type child_id = QuadTree::get_child_id(tile_id, n);

        if(_cut_db->get_tile_provider()->getTile(cut->get_atlas(), child_id, 100) == nullptr)
        {
            all_children_available = false;
            continue;
        }
    }

    bool all_children_added = true;

    if(all_children_available)
    {
        for(size_t n = 0; n < 4; n++)
        {
            id_type child_id = QuadTree::get_child_id(tile_id, n);
            uint8_t *tile_ptr = _cut_db->get_tile_provider()->getTile(cut->get_atlas(), child_id, 100)->getBuffer();

            if(tile_ptr == nullptr)
            {
                throw std::runtime_error("Child removed from RAM");
            }

            if(!add_to_indexed_memory(cut, child_id, tile_ptr))
            {
                all_children_added = false;
            }
        }
    }

    return all_children_available && all_children_added;
}
bool CutUpdate::keep_id(Cut *cut, id_type tile_id)
{
    ooc::TileCacheSlot *slot = _cut_db->get_tile_provider()->getTile(cut->get_atlas(), tile_id, 100);

    if(slot == nullptr)
    {
        throw std::runtime_error("kept tile #" + std::to_string(tile_id) + "not found in RAM");
    }

    uint8_t *tile_ptr = slot->getBuffer();

    if(tile_ptr == nullptr)
    {
        throw std::runtime_error("kept tile #" + std::to_string(tile_id) + "not found in RAM");
    }

    return add_to_indexed_memory(cut, tile_id, tile_ptr);
}
mem_slot_type *CutUpdate::read_mem_slot_for_id(Cut *cut, id_type tile_id)
{
    auto mem_slot_iter = cut->get_front()->get_mem_slots_locked().find(tile_id);

    if(mem_slot_iter == cut->get_front()->get_mem_slots_locked().end())
    {
        return nullptr;
    }

    return _cut_db->read_mem_slot_at((*mem_slot_iter).second);
}
mem_slot_type *CutUpdate::write_mem_slot_for_id(Cut *cut, id_type tile_id)
{
    auto mem_slot_iter = cut->get_back()->get_mem_slots_locked().find(tile_id);

    if(mem_slot_iter == cut->get_back()->get_mem_slots_locked().end())
    {
        return nullptr;
    }

    return _cut_db->write_mem_slot_at((*mem_slot_iter).second);
}
void CutUpdate::feedback(int32_t *buf_lod, uint32_t*buf_count)
{
    if(_dispatch_lock.try_lock())
    {
        std::copy(buf_lod, buf_lod + _cut_db->get_size_mem_interleaved(), _feedback_lod_buffer);
        std::copy(buf_count, buf_count + _cut_db->get_size_mem_interleaved(), _feedback_count_buffer);
        _new_feedback.store(true);
        _dispatch_lock.unlock();
        _cv.notify_one();
    }
}

void CutUpdate::stop()
{
    _should_stop.store(true);
    _worker.join();
}
uint8_t CutUpdate::count_children_in_cut(id_type tile_id, const cut_type &cut)
{
    uint8_t count = 0;
    for(uint8_t i = 0; i < 4; i++)
    {
        id_type child_id = QuadTree::get_child_id(tile_id, i);
        count += cut.count(child_id);
    }
    return count;
}
bool CutUpdate::check_all_siblings_in_cut(id_type tile_id, const cut_type &cut)
{
    id_type parent_id = QuadTree::get_parent_id(tile_id);
    return count_children_in_cut(parent_id, cut) == 4;
}
const float &CutUpdate::get_dispatch_time() const { return _dispatch_time; }
void CutUpdate::set_freeze_dispatch(bool _freeze_dispatch) { CutUpdate::_freeze_dispatch = _freeze_dispatch; }
bool CutUpdate::get_freeze_dispatch() { return _freeze_dispatch; }
void CutUpdate::remove_from_indexed_memory(Cut *cut, id_type tile_id)
{
    mem_slot_type *mem_slot = write_mem_slot_for_id(cut, tile_id);

    if(mem_slot != nullptr)
    {
        uint_fast32_t x_orig, y_orig;
        QuadTree::get_pos_by_id(tile_id, x_orig, y_orig);
        uint16_t tile_depth = QuadTree::get_depth_of_node(tile_id);

        uint8_t *ptr = &cut->get_back()->get_index(tile_depth)[(y_orig * QuadTree::get_tiles_per_row(tile_depth) + x_orig) * 4];
        ptr[3] = (uint8_t)0;

        cut->get_back()->get_mem_slots_updated().erase(mem_slot->tile_id);
        cut->get_back()->get_mem_slots_locked().erase(mem_slot->tile_id);

        _cut_db->get_tile_provider()->ungetTile(cut->get_atlas(), mem_slot->tile_id);

        mem_slot->locked = false;
        mem_slot->updated = false;
        mem_slot->tile_id = UINT64_MAX;
    }
    else
    {
        throw std::runtime_error("Collapsed child not in memory slots");
    }
}
}
