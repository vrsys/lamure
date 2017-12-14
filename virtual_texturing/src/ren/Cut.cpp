#include <lamure/vt/VTContext.h>
#include <lamure/vt/ren/Cut.h>

namespace vt
{
Cut::Cut(VTContext *context) : _front_cut(), _front_mem_cut(), _front_mem_slots_free(), _back_cut(), _back_mem_cut(), _back_mem_slots_free(), _swap_time()
{
    _size_index = context->get_size_index_texture() * context->get_size_index_texture() * 3;
    _size_mem_x = context->calculate_size_physical_texture().x;
    _size_mem_y = context->calculate_size_physical_texture().y;
    _size_feedback = _size_mem_x * _size_mem_y;

    _front_index = new uint8_t[_size_index];
    _back_index = new uint8_t[_size_index];

    std::fill(_front_index, _front_index + _size_index, 0);
    std::fill(_back_index, _back_index + _size_index, 0);

    _front_mem_slots = new id_type[_size_feedback];
    _back_mem_slots = new id_type[_size_feedback];

    std::fill(_front_mem_slots, _front_mem_slots + _size_feedback, UINT64_MAX);
    std::fill(_back_mem_slots, _back_mem_slots + _size_feedback, UINT64_MAX);

    for(size_t i = 0; i < _size_feedback; i++)
    {
        _front_mem_slots_free.insert(i);
        _back_mem_slots_free.insert(i);
    }

    _front_cut.insert(0);
    _back_cut.insert(0);

    _new_data = false;
}
Cut::~Cut()

{
    delete[] _front_index;
    delete[] _back_index;
    delete[] _front_mem_slots;
    delete[] _back_mem_slots;
}
void Cut::_swap()
{
    /*for(auto id : _back_cut){
        std::cout << id << " ";
    }

    std::cout << std::endl;*/

    auto start = std::chrono::high_resolution_clock::now();

    _front_cut = _back_cut;
    _front_mem_cut = _back_mem_cut;
    _front_mem_slots_free = _back_mem_slots_free;

    auto tmp_index = _back_index;
    _back_index = _front_index;
    _front_index = tmp_index;

    memcpy(_back_index, _front_index, _size_index);

    auto tmp_mem_slots = _back_mem_slots;
    _back_mem_slots = _front_mem_slots;
    _front_mem_slots = tmp_mem_slots;

    memcpy(_back_mem_slots, _front_mem_slots, _size_feedback);

    auto end = std::chrono::high_resolution_clock::now();

    _swap_time = std::chrono::duration<float, std::micro>(end - start).count();
}
void Cut::start_writing() { _back_lock.lock(); }
void Cut::stop_writing()
{
    if(_front_lock.try_lock())
    {
        _swap();
        _front_lock.unlock();

        _new_data = false;
    }
    else
    {
        _new_data = true;
    }

    _back_lock.unlock();
}
void Cut::start_reading()
{
    _front_lock.lock();

    if(_back_lock.try_lock())
    {
        if(_new_data)
        {
            _swap();
            _new_data = false;
        }

        _back_lock.unlock();
    }
}
void Cut::stop_reading() { _front_lock.unlock(); }

const size_t Cut::get_size_index() const { return _size_index; }
const size_t Cut::get_size_mem_x() const { return _size_mem_x; }
const size_t Cut::get_size_mem_y() const { return _size_mem_y; }
const size_t Cut::get_size_feedback() const { return _size_feedback; }

const std::set<id_type> &Cut::get_front_cut() const { return _front_cut; }
const uint8_t *Cut::get_front_index() const { return _front_index; }
const std::map<size_t, uint8_t *> &Cut::get_front_mem_cut() const { return _front_mem_cut; }
const id_type *Cut::get_front_mem_slots() const { return _front_mem_slots; }

std::set<id_type> &Cut::get_back_cut() { return _back_cut; }
uint8_t *Cut::get_back_index() { return _back_index; }
std::map<size_t, uint8_t *> &Cut::get_back_mem_cut() { return _back_mem_cut; }
id_type *Cut::get_back_mem_slots() { return _back_mem_slots; }
std::set<size_t> &Cut::get_back_mem_slots_free() { return _back_mem_slots_free; }
const mem_slots_free_type &Cut::get_front_mem_slots_free() const { return _front_mem_slots_free; }
const float &Cut::get_swap_time() const { return _swap_time; }
}