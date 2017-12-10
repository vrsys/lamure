#include <lamure/vt/ren/CutUpdate.h>
#include <lamure/vt/VTContext.h>

namespace vt
{
CutUpdate::CutUpdate(vt::VTContext *context, vt::TileAtlas<priority_type> *atlas) : _dispatch_lock(), _idx_buffer(context->get_size_index_texture() * context->get_size_index_texture() * 3)
{
    _cut = std::set<uint32_t>();
    _should_stop.store(false);
    _new_feedback.store(false);
    _feedback_buffer = nullptr;
    _atlas = atlas;

    _context = context;

    // TODO: adapt to phys. texture size
    _slots = new id_type[10];

    _slots[0] = 1;

    for(size_t i = 1; i < 10; ++i){
        _slots[i] = 0;
    }
}

    void CutUpdate::set_renderer(VTRenderer *renderer){
        _renderer = renderer;
    }

CutUpdate::~CutUpdate() {}
void CutUpdate::start() {
    _worker = std::thread(&CutUpdate::run, this);
    auto idx = _idx_buffer.startWriting();

    std::fill(idx, idx + _idx_buffer.getSize(), 0);

    _idx_buffer.stopWriting();
    idx = _idx_buffer.startWriting();

    std::fill(idx, idx + _idx_buffer.getSize(), 0);

    _idx_buffer.stopWriting();
}

void CutUpdate::run()
{
    while(!_should_stop.load())
    {
        // if(_new_feedback.load()) {
        std::unique_lock<std::mutex> lk(_dispatch_lock, std::defer_lock);
        if(_new_feedback.load()) dispatch();
        //_new_feedback.store(false);
        //}
        _new_feedback.store(false);

        while(!_cv.wait_for(lk, std::chrono::milliseconds(100), [this]{
            return _new_feedback.load();
        })){
            if(_should_stop.load()){
                break;
            }
        }
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

    /*void CutUpdate::dispatch()
{
    std::queue<uint32_t> queue_collapse = std::queue<uint32_t>();
    std::queue<uint32_t> queue_split = std::queue<uint32_t>();
    std::queue<uint32_t> queue_keep = std::queue<uint32_t>();

    for(uint32_t iter_old_cut : _cut)
    {
        if(this->check_siblings_in_cut(iter_old_cut, _cut))
        {
            if(interpret_feedback(iter_old_cut) > max_threshold)
            {
                queue_split.push(iter_old_cut);
            }
            else if(interpret_feedback(iter_old_cut) > min_threshold)
            {
                queue_collapse.push(iter_old_cut);
            }
            else
            {
                queue_keep.push(iter_old_cut);
            }
        }
        else
        {
            if(interpret_feedback(iter_old_cut) > max_threshold)
            {
                queue_split.push(iter_old_cut);
            }
            else
            {
                queue_keep.push(iter_old_cut);
            }
        }
    }

    std::set<uint32_t> _new_cut;

    while(!queue_collapse.empty())
    {
        auto _tile_id = queue_collapse.front();
        collapse_id(_tile_id);
        queue_collapse.pop();
        _new_cut.insert(_tile_id);
    }

    while(!queue_split.empty())
    {
        auto _tile_id = queue_split.front();
        if(memory_available_for_split())
        {
            for(uint8_t i = 0; i < 4; i++)
            {
                uint32_t _child_id = QuadTree::get_child_id(_tile_id, i);
                _new_cut.insert(_child_id);
            }
        }
        else
        {
            _new_cut.insert(_tile_id);
        }
        queue_split.pop();
    }

    while(!queue_keep.empty())
    {
        auto _tile_id = queue_keep.front();
        queue_keep.pop();
        _new_cut.insert(_tile_id);
    }
}*/

void CutUpdate::dispatch(){
    for(size_t i = 0; i < 10; ++i){
        auto tile_id = _slots[i];

        if(tile_id == 0){
            continue;
        }

        --tile_id;

        if(_feedback_buffer[i] > (256 * 256 * 2)){
            // split

            for(size_t n = 0; n < 4; ++n){
                auto child_id = QuadTree::get_child_id(tile_id, n);
                auto tile_ptr = _atlas->get(child_id, 100);

                if(tile_ptr != nullptr){
                    auto free_slot = get_free_slot();

                    if(free_slot != 0){
                        _slots[free_slot - 1] = child_id;

                        //auto idx_tex_size = _context->get_size_index_texture() * _context->get_size_index_texture() * 3;
                        //std::cout << idx_tex_size << std::endl;

                        uint8_t *idx = _idx_buffer.startWriting();

                        auto level = QuadTree::get_depth_of_node(child_id);
                        size_t x_pos;
                        size_t y_pos;

                        QuadTree::get_pos_by_id(child_id, x_pos, y_pos);
                        auto tile_width = _context->get_size_index_texture() / (1 << level);

                        std::cout << child_id << " " << x_pos << " " << y_pos << std::endl;

                        for(size_t x = x_pos * tile_width; x < (x_pos + 1) * tile_width; ++x){
                            for(size_t y = y_pos * tile_width; y < (y_pos + 1) * tile_width; ++y){
                                auto ptr = &idx[y * _context->get_size_index_texture() * 3 + x * 3];

                                ptr[0] = child_id;
                                ptr[1] = 0;
                                ptr[2] = level;
                            }
                        }

                        //uint8_t idx[512];

                        /*auto level = 0;
                        bool no_more_levels = true;

                        do{
                            no_more_levels = true;

                            for(size_t slot = 0; slot < 10; ++slot){
                                if(_slots[slot] != 0){
                                    auto slot_id = _slots[slot] - 1;
                                    auto slot_level = QuadTree::get_depth_of_node(slot_id);

                                    if(slot_level >= level){
                                        no_more_levels = false;
                                    }

                                    if(slot_level == level){
                                        size_t x_pos;
                                        size_t y_pos;

                                        QuadTree::get_pos_by_id(child_id, x_pos, y_pos);
                                        auto tile_width = _context->get_size_index_texture() / (1 << level);

                                        for(size_t x = x_pos * tile_width; x < (x_pos + 1) * tile_width; ++x){
                                            for(size_t y = y_pos * tile_width; y < (y_pos + 1) * tile_width; ++y){
                                                auto ptr = &idx[y * _context->get_size_index_texture() * 3 + x * 3];

                                                ptr[0] = slot_id;
                                                ptr[1] = 0;
                                                ptr[2] = slot_level;
                                            }
                                        }
                                    }
                                }
                            }

                            ++level;
                        }while(!no_more_levels);
*/

                        for(size_t x = 0; x < _context->get_size_index_texture(); ++x){
                            for(size_t y = 0; y < _context->get_size_index_texture(); ++y){
                                auto ptr = &idx[y * _context->get_size_index_texture() * 3 + x * 3];

                                std::cout << (int)ptr[0] << "." << (int)ptr[1] << "." << (int)ptr[2] << " ";
                            }

                            std::cout << std::endl;
                        }

                        _idx_buffer.stopWriting();

                        //delete[] idx;
                    }
                }
            }

            //std::cout << "split " << (_slots[i] - 1) << std::endl;
        }else if(_feedback_buffer[i] < (256 * 256 / 2) && _slots[i] > 0){
            // collapse

            //std::cout << "collapse " << (_slots[i] - 1) << std::endl;
        }else{
            // keep it like it is
        }
    }
}

    size_t CutUpdate::get_free_slot() {
        for(size_t i = 0; i < 10; ++i){
            if(_slots[i] == 0){
                return i + 1;
            }
        }

        return 0;
    }

void CutUpdate::feedback(uint32_t *buf)
{
    // if(!_new_feedback.load()) {
    std::unique_lock<std::mutex> lk(_dispatch_lock);
    this->_feedback_buffer = buf;
    //_new_feedback.store(true);
    _new_feedback.store(true);
    lk.unlock();
    _cv.notify_one();
    //}
}
void CutUpdate::stop()
{
    _should_stop.store(true);
    _worker.join();
}
bool CutUpdate::check_siblings_in_cut(const uint32_t _tile_id, std::set<uint32_t> &_cut)
{
    for(uint8_t i = 0; i < 4; i++)
    {
        uint32_t _child_id = QuadTree::get_child_id(_tile_id, i);
        if(_cut.find(_child_id) != _cut.cend())
        {
            return false;
        }
    }
    return true;
}
uint32_t CutUpdate::interpret_feedback(uint32_t _tile_id)
{
    // TODO
    return 0;
}
void CutUpdate::collapse_id(uint32_t _tile_id)
{
    // TODO
}
bool CutUpdate::memory_available_for_split()
{
    // TODO
    return false;
}
}