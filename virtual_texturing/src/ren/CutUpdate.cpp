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

    std::priority_queue<uint64_t, std::vector<uint64_t>, QuadTree::more_than_by_depth> queue_collapse;
    std::priority_queue<uint64_t, std::vector<uint64_t>, QuadTree::more_than_by_depth> queue_split;
    std::priority_queue<uint64_t, std::vector<uint64_t>, QuadTree::more_than_by_depth> queue_keep;
    std::map<id_type, size_t> map_children_in_cut;

    _cut.start_writing();

    for(auto iter = _cut.get_back_cut().rbegin(); iter != _cut.get_back_cut().rend(); ++iter)
    {
        auto tile_id = *iter;
        auto mem_slot = get_mem_slot_for_id(tile_id);

        if(mem_slot == SIZE_MAX && !_atlas->alreadyRequested(tile_id))
        {
            throw std::runtime_error("Shieeet");
        }

        auto parent_id = QuadTree::get_parent_id(tile_id);
        auto iter_parent = map_children_in_cut.find(parent_id);

        if(iter_parent == map_children_in_cut.end())
        {
            map_children_in_cut.insert(std::pair<int64_t, uint8_t>(parent_id, _feedback_buffer[mem_slot]));
        }
        else
        {
            iter_parent->second += _feedback_buffer[mem_slot];
        }

        auto iter_self = map_children_in_cut.find(tile_id);
        size_t sum_feedback = _feedback_buffer[mem_slot];

        if(iter_self != map_children_in_cut.end())
        {
            sum_feedback += iter_self->second;
        }

        if(texels_per_tile < _feedback_buffer[mem_slot] * 2.0f && QuadTree::get_depth_of_node(tile_id) < 4) //_context->get_depth_quadtree())
        {
            std::cout << "decision: split " << tile_id << ", " << _feedback_buffer[mem_slot] * 2.0f << " is over " << texels_per_tile << std::endl;
            queue_split.push(tile_id);
        }
        else if(texels_per_tile > (float)sum_feedback / 8.0f && check_all_siblings_in_cut(tile_id) && QuadTree::get_depth_of_node(tile_id) > 0)
        {
            std::cout << "decision: collapse " << tile_id << ", " << (float)sum_feedback / 8.0f << " is under " << texels_per_tile << std::endl;
            queue_collapse.push(tile_id);
        }
        else
        {
            // std::cout << "decision: keep, " << _feedback_buffer[i] << std::endl;
            queue_keep.push(tile_id);
        }
    }

    std::set<id_type> cut_new;

    while(!queue_collapse.empty())
    {
        // std::cout << "action: collapse" << std::endl;
        auto tile_id = queue_collapse.top();
        queue_collapse.pop();

        if(!collapse_id(tile_id, cut_new))
        {
            queue_keep.push(tile_id);
        }
    }

    while(!queue_split.empty())
    {
        auto tile_id = queue_split.top();
        queue_split.pop();

        if(memory_available_for_split())
        {
            // std::cout << "action: split" << std::endl;
            if(!split_id(tile_id, cut_new))
            {
                queue_keep.push(tile_id);
            }
        }
        else
        {
            // std::cout << "action: keep" << std::endl;
            queue_keep.push(tile_id);
        }
    }

    while(!queue_keep.empty())
    {
        // std::cout << "action: keep" << std::endl;
        auto tile_id = queue_keep.top();
        queue_keep.pop();

        keep_id(tile_id, cut_new);
    }

    if(cut_new.empty())
    {
        throw std::runtime_error("updated cut is empty");
    }

    _cut.get_back_updated_nodes().clear();

    std::set_difference(cut_new.begin(), cut_new.end(), _cut.get_back_cut().begin(), _cut.get_back_cut().end(), std::inserter(_cut.get_back_updated_nodes(), _cut.get_back_updated_nodes().begin()));

    _cut.get_back_cut().swap(cut_new);

    auto end = std::chrono::high_resolution_clock::now();
    _dispatch_time = std::chrono::duration<float, std::milli>(end - start).count();

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

    auto buf_idx = _cut.get_back_index();

    for(size_t x = x_orig * tile_span; x < (x_orig + 1) * tile_span; x++)
    {
        for(size_t y = y_orig * tile_span; y < (y_orig + 1) * tile_span; y++)
        {
            auto ptr = &buf_idx[(y * _context->get_size_index_texture() + x) * 4];

            size_t phys_tex_tile_width = _context->get_phys_tex_tile_width();
            size_t tiles_per_tex = phys_tex_tile_width * phys_tex_tile_width;
            size_t level = mem_slot / tiles_per_tex;
            size_t rel_pos = mem_slot - level * tiles_per_tex;
            size_t x_tile = rel_pos % phys_tex_tile_width;
            size_t y_tile = rel_pos / phys_tex_tile_width;

            ptr[0] = (uint8_t)x_tile;
            ptr[1] = (uint8_t)y_tile;
            ptr[2] = (uint8_t)tile_depth;
            ptr[3] = (uint8_t)level;
        }
    }

    _cut.get_back_mem_cut().insert(std::pair<size_t, uint8_t *>(mem_slot, tile_ptr));

    return true;
}

void CutUpdate::remove_from_indexed_memory(id_type tile_id)
{
    auto mem_index = get_mem_slot_for_id(tile_id);

    _cut.get_back_mem_slots()[mem_index] = UINT64_MAX;
    _cut.get_back_mem_slots_free().insert(mem_index);
}

bool CutUpdate::collapse_id(id_type tile_id, cut_type &cut_new)
{
    auto parent_id = QuadTree::get_parent_id(tile_id);
    auto tile_ptr = _atlas->get(parent_id, 100);

    if(tile_ptr == nullptr)
    {
        return false;
    }

    if(try_add_to_indexed_memory(parent_id, tile_ptr))
    {
        cut_new.insert(parent_id);
        return true;
    }

    return false;
}
bool CutUpdate::split_id(id_type tile_id, cut_type &cut_new)
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

            if(try_add_to_indexed_memory(child_id, tile_ptr))
            {
                cut_new.insert(child_id);
            }
            else
            {
                all_children_added = false;
            }
        }
    }

    return all_children_available && all_children_added;
}
void CutUpdate::keep_id(id_type tile_id, cut_type &cut_new)
{
    auto tile_ptr = _atlas->get(tile_id, 100);
    if(tile_ptr == nullptr)
    {
        throw std::runtime_error("kept tile #" + std::to_string(tile_id) + "not found in RAM");
    }

    if(try_add_to_indexed_memory(tile_id, tile_ptr))
    {
        cut_new.insert(tile_id);
    }
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
