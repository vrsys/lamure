#include <lamure/vt/VTContext.h>
#include <lamure/vt/ren/CutUpdate.h>

namespace vt
{
CutUpdate::CutUpdate(vt::VTContext *context) : _dispatch_lock(), _mem_slots_free()
{
    _cut = std::set<id_type>();
    _should_stop.store(false);
    _new_feedback.store(false);
    _feedback_buffer = nullptr;
    _atlas = context->get_atlas();

    _context = context;

    _size_feedback = _context->get_size_physical_texture() * _context->get_size_physical_texture();
    _mem_slots = new id_type[_size_feedback];

    _mem_slots[0] = 0;

    std::fill(_mem_slots, _mem_slots + _size_feedback, UINT64_MAX);
}

CutUpdate::~CutUpdate() {}

void CutUpdate::start()
{
    // request tile 0 and wait until it is there
    _atlas->get(0, 100);
    _atlas->wait();
    auto *root_tile = (char *)_atlas->get(0, 0);

    _size_buf_idx = _context->get_size_index_texture() * _context->get_size_index_texture() * 3;
    _buf_idx = new uint8_t[_size_buf_idx];
    std::fill(_buf_idx, _buf_idx + _size_buf_idx, 0);

    for(size_t i = 0; i < _size_feedback; i++)
    {
        _mem_slots_free.push(i);
    }

    _context->get_vtrenderer()->update_physical_texture_blockwise(root_tile, 0, 0);
    _context->get_vtrenderer()->update_index_texture(cpu_idx_texture_buffer_state);

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
    _size_feedback = _context->get_size_physical_texture() * _context->get_size_physical_texture();

    auto threshold_max = _context->get_size_tile() * _context->get_size_tile() * 2;
    auto threshold_min = _context->get_size_tile() * _context->get_size_tile() / 2;

    std::queue<uint64_t> queue_collapse = std::queue<uint64_t>();
    std::queue<uint64_t> queue_split = std::queue<uint64_t>();
    std::queue<uint64_t> queue_keep = std::queue<uint64_t>();

    for(size_t i = 0; i < _size_feedback; ++i)
    {
        if(_mem_slots[i] == UINT64_MAX)
        {
            continue;
        }

        auto tile_id = _mem_slots[i];

        if(_cut.find(tile_id) == _cut.cend())
        {
            continue;
        }

        if(this->check_children_in_cut(tile_id, _cut))
        {
            if(_feedback_buffer[i] > threshold_max)
            {
                queue_split.push(tile_id);
            }
            else if(_feedback_buffer[i] < threshold_min)
            {
                queue_collapse.push(tile_id);
            }
            else
            {
                queue_keep.push(tile_id);
            }
        }
        else
        {
            if(_feedback_buffer[i] > threshold_max)
            {
                queue_split.push(tile_id);
            }
            else
            {
                queue_keep.push(tile_id);
            }
        }
    }

    std::set<id_type> new_cut;

    while(!queue_collapse.empty())
    {
        auto _tile_id = queue_collapse.front();
        collapse_id(_tile_id);
        queue_collapse.pop();
        new_cut.insert(_tile_id);
    }

    while(!queue_split.empty())
    {
        auto _tile_id = queue_split.front();
        if(memory_available_for_split())
        {
            for(uint8_t i = 0; i < 4; i++)
            {
                id_type _child_id = QuadTree::get_child_id(_tile_id, i);
                new_cut.insert(_child_id);
            }
        }
        else
        {
            new_cut.insert(_tile_id);
        }
        queue_split.pop();
    }

    while(!queue_keep.empty())
    {
        auto _tile_id = queue_keep.front();
        queue_keep.pop();
        new_cut.insert(_tile_id);
    }
}

void CutUpdate::collapse_id(id_type tile_id)
{
    // TODO
}

// add children if the memory is available, otherwise keep
void CutUpdate::split_id(id_type tile_id, size_t size_feedback)
{
    if(memory_available_for_split())
    {
        return;
    }

    for(size_t n = 0; n < 4; ++n)
    {
        auto child_id = QuadTree::get_child_id(tile_id, n);
        auto tile_ptr = _atlas->get(child_id, 100);

        if(tile_ptr == nullptr)
            continue;

        auto free_slot = get_free_mem_slot();

        if(free_slot == SIZE_MAX)
            break;

        _mem_slots[free_slot] = child_id;

        size_t x, y;

        QuadTree::get_pos_by_id(child_id, x, y);

        auto ptr = &_buf_idx[y * _context->get_size_index_texture() * 3 + x * 3];

        auto slot_level = QuadTree::get_depth_of_node(child_id);

        if(ptr[2] > slot_level)
        {
            continue;
        }

        ptr[0] = 0;
        ptr[1] = 0;
        // TODO: crazy shit
        ptr[2] = static_cast<uint8_t>(slot_level);

        //        for(size_t x = 0; x < _context->get_size_index_texture(); ++x)
        //        {
        //            for(size_t y = 0; y < _context->get_size_index_texture(); ++y)
        //            {
        //                auto ptr = &idx[y * _context->get_size_index_texture() * 3 + x * 3];
        //
        //                std::cout << (int)ptr[0] << "." << (int)ptr[1] << "." << (int)ptr[2] << " ";
        //            }
        //
        //            std::cout << std::endl;
        //        }
    }
}

// check if 4 nodes can be created in memory
bool CutUpdate::memory_available_for_split() { return !_mem_slots_free.size() < 4; } // NOLINT
size_t CutUpdate::get_free_mem_slot() { return !_mem_slots_free.empty() ? _mem_slots_free.pop() : SIZE_MAX; }
void CutUpdate::feedback(uint32_t *buf)
{
    std::unique_lock<std::mutex> lk(_dispatch_lock);
    this->_feedback_buffer = buf;
    _new_feedback.store(true);
    lk.unlock();
    _cv.notify_one();
}
void CutUpdate::stop()
{
    _should_stop.store(true);
    _worker.join();
}
bool CutUpdate::check_children_in_cut(id_type tile_id, std::set<id_type> &cut)
{
    for(uint8_t i = 0; i < 4; i++)
    {
        id_type _child_id = QuadTree::get_child_id(tile_id, i);
        if(cut.find(_child_id) != cut.cend())
        {
            return false;
        }
    }
    return true;
}
}