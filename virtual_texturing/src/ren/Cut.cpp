#include <lamure/vt/VTContext.h>
#include <lamure/vt/ren/Cut.h>

namespace vt
{
Cut::Cut(VTContext *context)
    : _front_cut(), _front_mem_slots(), _front_mem_slots_locked(), _front_mem_slots_updated(), _back_cut(), _back_mem_slots(), _back_mem_slots_locked(), _back_mem_slots_updated(), _deliver_time()
{
    _size_index = context->get_size_index_texture() * context->get_size_index_texture() * 4;
    _size_mem_x = context->get_phys_tex_tile_width();
    _size_mem_y = context->get_phys_tex_tile_width();
    _size_feedback = _size_mem_x * _size_mem_y * context->get_phys_tex_layers();

    _front_index = new uint8_t[_size_index];
    _back_index = new uint8_t[_size_index];

    std::fill(_front_index, _front_index + _size_index, 0);
    std::fill(_back_index, _back_index + _size_index, 0);

    for(size_t i = 0; i < _size_feedback; i++)
    {
        _front_mem_slots.emplace_back(mem_slot_type{i, UINT64_MAX, nullptr, false, false});
        _back_mem_slots.emplace_back(mem_slot_type{i, UINT64_MAX, nullptr, false, false});
    }

    _front_cut.insert(0);
    _back_cut.insert(0);

    _new_data = false;
}
Cut::~Cut()
{
    delete[] _front_index;
    delete[] _back_index;
}
void Cut::deliver()
{
    auto start = std::chrono::high_resolution_clock::now();

    _front_cut.clear();
    _front_cut.insert(_back_cut.begin(), _back_cut.end());

    _front_mem_slots.assign(_back_mem_slots.begin(), _back_mem_slots.end());

    _front_mem_slots_locked = mem_slots_index_type(_back_mem_slots_locked);
    _front_mem_slots_updated = mem_slots_index_type(_back_mem_slots_updated);

    std::copy(_back_index, _back_index + _size_index, _front_index);

    auto end = std::chrono::high_resolution_clock::now();

    _deliver_time = std::chrono::duration<float, std::micro>(end - start).count();
}
void Cut::start_writing() { _back_lock.lock(); }
void Cut::stop_writing()
{
    if(_front_lock.try_lock())
    {
        deliver();
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
            deliver();
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
const std::mutex &Cut::get_front_lock() const { return _front_lock; }
const cut_type &Cut::get_front_cut() const { return _front_cut; }
uint8_t *Cut::get_front_index() const { return _front_index; }
const mem_slots_type &Cut::get_front_mem_slots() const { return _front_mem_slots; }
std::mutex &Cut::get_back_lock() { return _back_lock; }
cut_type &Cut::get_back_cut() { return _back_cut; }
uint8_t *Cut::get_back_index() { return _back_index; }
mem_slots_type &Cut::get_back_mem_slots() { return _back_mem_slots; }
float Cut::get_deliver_time() const { return _deliver_time; }
const mem_slots_index_type &Cut::get_front_mem_slots_updated() const { return _front_mem_slots_updated; }
const mem_slots_index_type &Cut::get_front_mem_slots_locked() const { return _front_mem_slots_locked; }
mem_slots_index_type &Cut::get_back_mem_slots_updated() { return _back_mem_slots_updated; }
mem_slots_index_type &Cut::get_back_mem_slots_locked() { return _back_mem_slots_locked; }
}