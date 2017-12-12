#include <lamure/vt/VTContext.h>
#include <lamure/vt/ren/CutUpdate.h>
#include <lamure/vt/ren/VTRenderer.h>

namespace vt
{
CutUpdate::CutUpdate(vt::VTContext *context) : _dispatch_lock(), _cut(context)
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
    auto *root_tile = _atlas->get(0, 0);

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
    auto texels_per_tile = _context->get_size_tile() * _context->get_size_tile();

    std::priority_queue<uint64_t, std::vector<uint64_t>, QuadTree::less_then_by_depth> queue_collapse;
    std::priority_queue<uint64_t, std::vector<uint64_t>, QuadTree::less_then_by_depth> queue_split;
    std::priority_queue<uint64_t, std::vector<uint64_t>, QuadTree::less_then_by_depth> queue_keep;

    _cut.start_writing();

    for(size_t i = 0; i < _cut.get_size_feedback(); ++i)
    {
        if(_cut.get_back_mem_slots()[i] == UINT64_MAX)
        {
            continue;
        }

        auto tile_id = _cut.get_back_mem_slots()[i];

        if(_cut.get_back_cut().find(tile_id) == _cut.get_back_cut().cend())
        {
            continue;
        }

        uint8_t children_in_cut = count_children_in_cut(tile_id, _cut.get_back_cut());

        if(children_in_cut > 0)
        {
            if((1.0f - (children_in_cut / 4.0f)) * texels_per_tile > 2 * _feedback_buffer[i] && QuadTree::get_depth_of_node(tile_id) < _context->get_depth_quadtree())
            {
                std::cout << "decision: split, " << (1.0f - (children_in_cut / 4.0f)) * texels_per_tile << " is over " << 2 * _feedback_buffer[i] << std::endl;
                queue_split.push(tile_id);
            }
            else if((1.0f - (children_in_cut / 4.0f)) * texels_per_tile < 0.5f * _feedback_buffer[i] && tile_id > 0)
            {
                std::cout << "decision: collapse, " << (1.0f - (children_in_cut / 4.0f)) * texels_per_tile << " is under " << 0.5f * _feedback_buffer[i] << std::endl;
                queue_collapse.push(tile_id);
            }
            else
            {
                std::cout << "decision: keep, " << _feedback_buffer[i] << std::endl;
                queue_keep.push(tile_id);
            }
        }
        else
        {
            if(texels_per_tile > 2 * _feedback_buffer[i] && QuadTree::get_depth_of_node(tile_id) < _context->get_depth_quadtree())
            {
                std::cout << "decision: split, " << texels_per_tile << " is over " << 2 * _feedback_buffer[i] << std::endl;
                queue_split.push(tile_id);
            }
            else
            {
                std::cout << "decision: keep, " << _feedback_buffer[i] << std::endl;
                queue_keep.push(tile_id);
            }
        }
    }

    std::set<id_type> cut_new;
    // std::queue<std::pair<size_t, uint8_t *>>().swap(_cut.get_back_mem_cut());

    while(!queue_collapse.empty())
    {
        std::cout << "action: collapse" << std::endl;
        auto tile_id = queue_collapse.top();
        queue_collapse.pop();

        if(!collapse_id(tile_id, cut_new))
        {
            keep_id(tile_id, cut_new);
            std::cout << tile_id << std::endl;
        }
        // keep_id(tile_id, cut_new);
    }

    while(!queue_split.empty())
    {
        auto tile_id = queue_split.top();
        queue_split.pop();

        if(memory_available_for_split())
        {
            std::cout << "action: split" << std::endl;
            if(!split_id(tile_id, cut_new))
            {
                keep_id(tile_id, cut_new);
            }
        }
        else
        {
            std::cout << "action: keep" << std::endl;
            keep_id(tile_id, cut_new);
        }
    }

    while(!queue_keep.empty())
    {
        std::cout << "action: keep" << std::endl;
        auto tile_id = queue_keep.top();
        queue_keep.pop();

        keep_id(tile_id, cut_new);
    }

    // TODO: remove
    std::cout << "Cut { ";
    for(id_type iter : _cut.get_back_cut())
    {
        std::cout << iter << " ";
    }
    std::cout << "}" << std::endl;

    // TODO: remove
    for(size_t x = 0; x < _cut.get_size_mem_x(); ++x)
    {
        for(size_t y = 0; y < _cut.get_size_mem_y(); ++y)
        {
            if(_cut.get_back_mem_slots()[x + y * _cut.get_size_mem_x()] == UINT64_MAX)
            {
                std::cout << "F ";
            }
            else
            {
                std::cout << _cut.get_back_mem_slots()[x + y * _cut.get_size_mem_x()] << " ";
            }
        }

        std::cout << std::endl;
    }
    std::cout << std::endl;

    // TODO: remove
    for(size_t x = 0; x < _context->get_size_index_texture(); ++x)
    {
        for(size_t y = 0; y < _context->get_size_index_texture(); ++y)
        {
            auto ptr = &_cut.get_back_index()[y * _context->get_size_index_texture() * 3 + x * 3];

            std::cout << (int)ptr[0] << "." << (int)ptr[1] << "." << (int)ptr[2] << " ";
        }

        std::cout << std::endl;
    }
    std::cout << std::endl;

    if(cut_new.empty())
    {
        throw std::runtime_error("updated cut is empty");
    }

    //    if(cut_new.size() != _cut.get_back_mem_cut().size())
    //    {
    //        throw std::runtime_error("physical memory not aligned with the cut");
    //    }

    _cut.get_back_cut().swap(cut_new);
    _cut.stop_writing();
}

bool CutUpdate::try_add_to_indexed_memory(id_type tile_id, uint8_t *tile_ptr)
{
    auto mem_slot = get_mem_slot_for_id(tile_id);

    if(mem_slot == SIZE_MAX)
    {
        auto free_mem_slot_index = get_free_mem_slot_index();

        if(free_mem_slot_index == SIZE_MAX)
        {
            throw std::runtime_error("out of mem slots");
        }

        _cut.get_back_mem_slots()[free_mem_slot_index] = tile_id;
        _cut.get_back_mem_slots_free().erase(free_mem_slot_index);

        mem_slot = free_mem_slot_index;
    }

    size_t x_orig, y_orig;
    QuadTree::get_pos_by_id(tile_id, x_orig, y_orig);
    auto tile_depth = QuadTree::get_depth_of_node(tile_id);
    auto tile_span = _context->get_size_index_texture() >> tile_depth;

    // std::cout << tile_id << " " << x_orig << y_orig << std::endl;

    auto buf_idx = _cut.get_back_index();

    for(size_t x = x_orig; x < (x_orig + tile_span); x++)
    {
        for(size_t y = y_orig; y < (y_orig + tile_span); y++)
        {
            auto ptr = &buf_idx[y * _context->get_size_index_texture() * 3 + x * 3];

            ptr[0] = (uint8_t)(mem_slot % _cut.get_size_mem_x());
            ptr[1] = (uint8_t)(mem_slot / _cut.get_size_mem_x());
            ptr[2] = (uint8_t)tile_depth;
        }
    }

    _cut.get_back_mem_cut().push(std::pair<size_t, uint8_t *>(mem_slot, tile_ptr));

    return true;
}

// check if 4 nodes can be created in memory
void CutUpdate::remove_from_indexed_memory(id_type tile_id)
{
    auto mem_index = get_mem_slot_for_id(tile_id);

    _cut.get_back_mem_slots()[mem_index] = UINT64_MAX;
    _cut.get_back_mem_slots_free().insert(mem_index);
}

// remove children from memory
bool CutUpdate::collapse_id(id_type tile_id, std::set<id_type> &cut_new)
{
    auto parent_id = QuadTree::get_parent_id(tile_id);
    auto tile_ptr = _atlas->get(parent_id, 100);

    if(tile_ptr == nullptr)
    {
        return false;
    }

    try_add_to_indexed_memory(parent_id, tile_ptr);
    cut_new.insert(parent_id);

    return true;
}
// add children if the memory is available
bool CutUpdate::split_id(id_type tile_id, std::set<id_type> &cut_new)
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

        try_add_to_indexed_memory(child_id, tile_ptr);
        cut_new.insert(child_id);
    }

    return all_children_available;
}
void CutUpdate::keep_id(id_type tile_id, std::set<id_type> &cut_new)
{
    auto tile_ptr = _atlas->get(tile_id, 100);
    if(tile_ptr == nullptr)
    {
        throw std::runtime_error("kept tile #" + std::to_string(tile_id) + "not found in RAM");
    }

    //    auto mem_iter = std::find(_cut.get_back_mem_slots(), _cut.get_back_mem_slots() + _cut.get_size_feedback(), tile_id);
    //    if(mem_iter == _cut.get_back_mem_slots() + _cut.get_size_feedback())
    //    {
    //        throw std::runtime_error("kept tile #" + std::to_string(tile_id) + " not found in physical memory");
    //    }
    //    auto mem_index = (size_t)std::distance(_cut.get_back_mem_slots(), mem_iter);

    try_add_to_indexed_memory(tile_id, tile_ptr);
    cut_new.insert(tile_id);
}
bool CutUpdate::memory_available_for_split() { return !_cut.get_back_mem_slots_free().size() < 4; } // NOLINT
size_t CutUpdate::get_free_mem_slot_index()
{
    if(!_cut.get_back_mem_slots_free().empty())
    {
        size_t free_mem_slot = *_cut.get_back_mem_slots_free().begin();
        _cut.get_back_mem_slots_free().erase(free_mem_slot);
        return free_mem_slot;
    }
    else
    {
        return SIZE_MAX;
    }
}
size_t CutUpdate::get_mem_slot_for_id(id_type tile_id)
{
    auto mem_iter = std::find(_cut.get_back_mem_slots(), _cut.get_back_mem_slots() + _cut.get_size_feedback(), tile_id);

    if(mem_iter == _cut.get_back_mem_slots() + _cut.get_size_feedback())
    {
        return SIZE_MAX;
    }

    return (size_t)std::distance(_cut.get_back_mem_slots(), mem_iter);
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
uint8_t CutUpdate::count_children_in_cut(id_type tile_id, const std::set<id_type> &cut)
{
    uint8_t count = 0;
    for(uint8_t i = 0; i < 4; i++)
    {
        id_type child_id = QuadTree::get_child_id(tile_id, i);
        if(cut.find(child_id) != cut.cend())
        {
            count++;
        }
    }
    return count;
}
Cut *CutUpdate::start_reading_cut()
{
    _cut.start_reading();
    return &_cut;
}

void CutUpdate::stop_reading_cut() { _cut.stop_reading(); }
}
