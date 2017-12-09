#include <lamure/vt/ren/CutUpdate.h>

CutUpdate::CutUpdate()
{
    _cut = std::set<uint32_t>();
    _should_stop.store(false);
}
CutUpdate::~CutUpdate() {}
void CutUpdate::start() { _worker = std::thread(&CutUpdate::run, this); }
void CutUpdate::run()
{
    std::unique_lock<std::mutex> lk(_dispatch_lock);

    while(!_should_stop.load())
    {
        dispatch();
        _cv.wait(lk);
    }
}
void CutUpdate::feedback(std::vector<uint32_t> &buf)
{
    // TODO
    std::lock_guard<std::mutex> lk(_dispatch_lock);
    _cv.notify_one();
}
void CutUpdate::dispatch()
{
    std::queue<uint32_t> queue_collapse = std::queue<uint32_t>();
    std::queue<uint32_t> queue_split = std::queue<uint32_t>();
    std::queue<uint32_t> queue_keep = std::queue<uint32_t>();

    for(uint32_t iter_old_cut : _cut)
    {
        if(this->check_siblings_in_cut(iter_old_cut, _cut))
        {
            if(calculate_error_id(iter_old_cut) > max_threshold)
            {
                queue_split.push(iter_old_cut);
            }
            else if(calculate_error_id(iter_old_cut) > min_threshold)
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
            if(calculate_error_id(iter_old_cut) > max_threshold)
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
}
void CutUpdate::stop()
{
    std::unique_lock<std::mutex> lk(_dispatch_lock);
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
uint32_t CutUpdate::calculate_error_id(uint32_t _tile_id)
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
