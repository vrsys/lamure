#include <lamure/vt/VTContext.h>
#include <lamure/vt/ren/CutUpdate.h>
#include <lamure/vt/ren/VTRenderer.h>

namespace vt
{
CutUpdate::CutUpdate(vt::VTContext *context) : _dispatch_lock(), _cut(context), _dispatch_time()
{
    _should_stop.store(false);
    _new_feedback.store(false);
    _feedback_buffer = nullptr;
    _atlas = context->get_atlas();
    _context = context;
}

CutUpdate::~CutUpdate() {}

void CutUpdate::start()
{
    _atlas->get(0, 100);
    _atlas->wait();
    auto *root_tile = _atlas->get(0, 100);

    _cut.start_writing();
    try_add_to_indexed_memory(0, root_tile);
    _cut.stop_writing();

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

    auto start = std::chrono::high_resolution_clock::now();

    auto texels_per_tile = _context->get_size_tile() * _context->get_size_tile();

    ordered_quadtree_set_type collapse_to;
    ordered_quadtree_set_type split;
    ordered_quadtree_set_type keep;

    std::map<id_type, double> map_feedback_prop;

    _cut.start_writing();

    /* FEEDBACK PROPAGATION PASS */

    cut_type cut_desired = cut_type(_cut.get_back_cut());

    for(auto iter = cut_desired.crbegin(); iter != cut_desired.crend(); iter++)
    {
        id_type tile_id = *iter;
        mem_slot_type *mem_slot = get_mem_slot_for_id(tile_id);

        if(mem_slot == nullptr)
        {
            std::cerr << "Node " << std::to_string(tile_id) << " not found in memory slots" << std::endl;
            continue;
        }

        id_type parent_id = QuadTree::get_parent_id(tile_id);
        auto iter_parent = map_feedback_prop.find(parent_id);

        if(iter_parent == map_feedback_prop.end())
        {
            map_feedback_prop.insert(std::pair<id_type, double>(parent_id, _feedback_buffer[mem_slot->position]));
        }
        else
        {
            iter_parent->second += _feedback_buffer[mem_slot->position];
        }
    }

    /* DECISION MAKING PASS */

    for(auto iter = _cut.get_back_cut().crbegin(); iter != _cut.get_back_cut().crend(); iter++)
    {
        id_type tile_id = *iter;
        mem_slot_type *mem_slot = get_mem_slot_for_id(tile_id);

        if(mem_slot == nullptr)
        {
            std::cerr << "Node " << std::to_string(tile_id) << " not found in memory slots" << std::endl;
            continue;
        }

        id_type parent_id = QuadTree::get_parent_id(tile_id);
        auto iter_parent = map_feedback_prop.find(parent_id);

        if(texels_per_tile < (_feedback_buffer[mem_slot->position] * 2.5) && QuadTree::get_depth_of_node(tile_id) < _context->get_depth_quadtree())
        {
            // std::cout << "decision: split " << tile_id << ", " << _feedback_buffer[mem_slot_type] * 4.0 << " is over " << texels_per_tile << std::endl;

            cut_desired.erase(tile_id);

            for(uint8_t i = 0; i < 4; i++)
            {
                cut_desired.insert(QuadTree::get_child_id(tile_id, i));
            }

            split.insert(tile_id);
        }
        else if(texels_per_tile > (iter_parent->second * 4 * 2.5) && check_all_siblings_in_cut(tile_id) && QuadTree::get_depth_of_node(tile_id) > 0)
        {
            // std::cout << "decision: collapse to " << parent_id << ", " << iter_parent->second * 4.0 << " is under " << texels_per_tile << std::endl;

            for(uint8_t i = 0; i < 4; i++)
            {
                cut_desired.erase(QuadTree::get_child_id(parent_id, i));
            }

            cut_desired.insert(parent_id);

            collapse_to.insert(parent_id);
        }
        else
        {
            // std::cout << "decision: keep " << tile_id << ", " << _feedback_buffer[mem_slot_type] << std::endl;

            keep.insert(tile_id);
        }
    }

    /* BUDGET PASS */

    {
        const uint16_t COLLAPSE_BUDGET = 4;
        const uint16_t SPLIT_BUDGET = 4;

        while(collapse_to.size() > COLLAPSE_BUDGET)
        {
            // std::cout << "budgeting: collapse has " << collapse_to.size() << ", budget is " << COLLAPSE_BUDGET << std::endl;
            keep.insert(*--collapse_to.cend());
            collapse_to.erase(--collapse_to.cend());
        }

        while(split.size() > SPLIT_BUDGET)
        {
            // std::cout << "budgeting: split has " << split.size() << ", budget is " << SPLIT_BUDGET << std::endl;
            keep.insert(*split.cbegin());
            split.erase(split.cbegin());
        }

        free_memory_for_split(SPLIT_BUDGET);
    }

    /* MEMORY INDEXING PASS */

    for(id_type tile_id : collapse_to)
    {
        // std::cout << "action: collapse to " << tile_id << std::endl;
        if(!collapse_to_id(tile_id))
        {
            for(uint8_t i = 0; i < 4; i++)
            {
                keep.insert(QuadTree::get_child_id(tile_id, i));
            }
        }
    }

    for(id_type tile_id : split)
    {
        if(memory_available_for_split(1))
        {
            // std::cout << "action: split " << tile_id << std::endl;
            if(!split_id(tile_id))
            {
                keep.insert(tile_id);
            }
        }
        else
        {
            if(free_memory_for_split(1))
            {
                // std::cout << "action: split " << tile_id << std::endl;
                if(!split_id(tile_id))
                {
                    keep.insert(tile_id);
                }
            }
            else
            {
                keep.insert(tile_id);
            }
        }
    }

    for(id_type tile_id : keep)
    {
        // std::cout << "action: keep " << tile_id << std::endl;
        keep_id(tile_id);
    }

    /* OPTIMAL UPDATE PASS */

    cut_type cut_new;

    identify_effective_cut(cut_new);

    //    for(size_t updated_mem_slot : _cut.get_back_updated_mem_slots())
    //    {
    //        std::cout << "updated_mem_slot: " << updated_mem_slot << std::endl;
    //    }

    if(cut_new.empty())
    {
        throw std::runtime_error("updated cut is empty");
    }

    _cut.get_back_cut().swap(cut_new);

    auto end = std::chrono::high_resolution_clock::now();
    _dispatch_time = std::chrono::duration<float, std::milli>(end - start).count();

    _cut.stop_writing();
}

bool CutUpdate::try_add_to_indexed_memory(id_type tile_id, uint8_t *tile_ptr)
{
    mem_slot_type *mem_slot = get_mem_slot_for_id(tile_id);

    if(mem_slot == nullptr)
    {
        mem_slot_type *free_mem_slot = get_free_mem_slot();

        if(free_mem_slot == nullptr)
        {
            throw std::runtime_error("out of mem slots");
        }

        free_mem_slot->locked = true;
        free_mem_slot->tile_id = tile_id;
        free_mem_slot->pointer = tile_ptr;

        mem_slot = free_mem_slot;
    }

    uint32_t x_orig, y_orig;
    QuadTree::get_pos_by_id(tile_id, x_orig, y_orig);
    uint16_t tile_depth = QuadTree::get_depth_of_node(tile_id);
    uint32_t tile_span = _context->get_size_index_texture() >> tile_depth;

    uint8_t *buf_idx = _cut.get_back_index();

    size_t phys_tex_tile_width = _context->get_phys_tex_tile_width();
    size_t tiles_per_tex = phys_tex_tile_width * phys_tex_tile_width;

    for(size_t x = x_orig * tile_span; x < (x_orig + 1) * tile_span; x++)
    {
        for(size_t y = y_orig * tile_span; y < (y_orig + 1) * tile_span; y++)
        {
            uint8_t *ptr = &buf_idx[(y * _context->get_size_index_texture() + x) * 4];

            size_t level = mem_slot->position / tiles_per_tex;
            size_t rel_pos = mem_slot->position - level * tiles_per_tex;
            size_t x_tile = rel_pos % phys_tex_tile_width;
            size_t y_tile = rel_pos / phys_tex_tile_width;

            ptr[0] = (uint8_t)x_tile;
            ptr[1] = (uint8_t)y_tile;
            ptr[2] = (uint8_t)tile_depth;
            ptr[3] = (uint8_t)level;
        }
    }

    return true;
}

void CutUpdate::identify_effective_cut(cut_type &cut)
{
    size_t phys_tex_tile_width = _context->get_phys_tex_tile_width();
    size_t tiles_per_tex = phys_tex_tile_width * phys_tex_tile_width;

    auto buf_idx = _cut.get_back_index();

    for(size_t x = 0; x < _context->get_size_index_texture(); x++)
    {
        for(size_t y = 0; y < _context->get_size_index_texture(); y++)
        {
            auto ptr = &buf_idx[(y * _context->get_size_index_texture() + x) * 4];

            uint8_t x_tile, y_tile, /*tile_depth, */ level;

            x_tile = ptr[0];
            y_tile = ptr[1];
            // tile_depth = ptr[2];
            level = ptr[3];

            size_t rel_pos = y_tile * phys_tex_tile_width + x_tile;
            size_t mem_slot_position = rel_pos + level * tiles_per_tex;

            mem_slot_type *mem_slot = get_mem_slot_for_position(mem_slot_position);
            mem_slot->locked = true;
            cut.insert(mem_slot->tile_id);
        }
    }
}

bool CutUpdate::collapse_to_id(id_type tile_id)
{
    uint8_t *tile_ptr = _atlas->get(tile_id, 100);

    if(tile_ptr == nullptr)
    {
        return false;
    }

    return try_add_to_indexed_memory(tile_id, tile_ptr);
}
bool CutUpdate::split_id(id_type tile_id)
{
    bool all_children_available = true;

    for(size_t n = 0; n < 4; n++)
    {
        auto child_id = QuadTree::get_child_id(tile_id, n);
        auto tile_ptr = _atlas->get(child_id, 100);

        if(tile_ptr == nullptr)
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
            auto child_id = QuadTree::get_child_id(tile_id, n);
            auto tile_ptr = _atlas->get(child_id, 100);

            if(tile_ptr == nullptr)
            {
                throw std::runtime_error("Child removed from RAM");
            }

            if(!try_add_to_indexed_memory(child_id, tile_ptr))
            {
                all_children_added = false;
            }
        }
    }

    return all_children_available && all_children_added;
}
bool CutUpdate::keep_id(id_type tile_id)
{
    auto tile_ptr = _atlas->get(tile_id, 100);
    if(tile_ptr == nullptr)
    {
        throw std::runtime_error("kept tile #" + std::to_string(tile_id) + "not found in RAM");
    }

    return try_add_to_indexed_memory(tile_id, tile_ptr);
}
bool CutUpdate::memory_available_for_split(uint32_t split_budget)
{
    size_t count = 0;

    for(auto mem_iter = _cut.get_back_mem_slots().begin(); mem_iter != _cut.get_back_mem_slots().end(); mem_iter++)
    {
        if(!(*mem_iter).locked)
        {
            count++;
            if(count > 4 * split_budget)
            {
                break;
            }
        }
    }
    return count > 4 * split_budget;
}
bool CutUpdate::free_memory_for_split(uint32_t split_budget)
{
    size_t count = 0;

    for(auto mem_iter = _cut.get_back_mem_slots().begin(); mem_iter != _cut.get_back_mem_slots().end(); mem_iter++)
    {
        if(mem_iter->locked && _cut.get_back_cut().find(mem_iter->tile_id)==_cut.get_back_cut().end())
        {
            mem_iter->locked = false;
            _atlas->unget(mem_iter->tile_id);

            count++;
            if(count > 4 * split_budget)
            {
                break;
            }
        }
    }
    return count > 4 * split_budget;
}
mem_slot_type *CutUpdate::get_free_mem_slot()
{
    for(auto mem_iter = _cut.get_back_mem_slots().begin(); mem_iter != _cut.get_back_mem_slots().end(); mem_iter++)
    {
        if(!(*mem_iter).locked)
        {
            return &(*mem_iter);
        }
    }

    throw std::runtime_error("out of mem slots");
}
mem_slot_type *CutUpdate::get_mem_slot_for_id(id_type tile_id)
{
    for(auto mem_iter = _cut.get_back_mem_slots().begin(); mem_iter != _cut.get_back_mem_slots().end(); mem_iter++)
    {
        if((*mem_iter).tile_id == tile_id)
        {
            return &(*mem_iter);
        }
    }

    return nullptr;
}
mem_slot_type *CutUpdate::get_mem_slot_for_position(size_t position)
{
    for(auto mem_iter = _cut.get_back_mem_slots().begin(); mem_iter != _cut.get_back_mem_slots().end(); mem_iter++)
    {
        if((*mem_iter).position == position)
        {
            return &(*mem_iter);
        }
    }

    return nullptr;
}
void CutUpdate::feedback(uint32_t *buf)
{
    std::unique_lock<std::mutex> lk(_dispatch_lock);
    _feedback_buffer = buf;
    _new_feedback.store(true);
    lk.unlock();
    _cv.notify_one();
}

void CutUpdate::stop()
{
    _should_stop.store(true);
    _worker.join();
}
uint8_t CutUpdate::count_children_in_cut(id_type tile_id)
{
    uint8_t count = 0;
    for(uint8_t i = 0; i < 4; i++)
    {
        id_type child_id = QuadTree::get_child_id(tile_id, i);
        count += _cut.get_back_cut().count(child_id);
    }
    return count;
}
bool CutUpdate::check_all_siblings_in_cut(id_type tile_id)
{
    id_type parent_id = QuadTree::get_parent_id(tile_id);
    return count_children_in_cut(parent_id) == 4;
}
Cut *CutUpdate::start_reading_cut()
{
    _cut.start_reading();
    return &_cut;
}

void CutUpdate::stop_reading_cut() { _cut.stop_reading(); }
const float &CutUpdate::get_dispatch_time() const { return _dispatch_time; }
void CutUpdate::set_freeze_dispatch(bool _freeze_dispatch) { CutUpdate::_freeze_dispatch = _freeze_dispatch; }
bool CutUpdate::get_freeze_dispatch() { return _freeze_dispatch; }
}
