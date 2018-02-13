//
// Created by towe2387 on 12/10/17.
//

#ifndef LAMURE_CUT_H
#define LAMURE_CUT_H

#include <lamure/vt/common.h>
namespace vt
{
class VTContext;
class Cut
{
  public:
    explicit Cut(VTContext *context);
    ~Cut();

    void start_writing();
    void stop_writing();

    void start_reading();
    void stop_reading();

    const size_t get_size_index() const;
    const size_t get_size_mem_x() const;
    const size_t get_size_mem_y() const;
    const size_t get_size_feedback() const;

    const std::mutex &get_front_lock() const;
    const cut_type &get_front_cut() const;
    uint8_t *get_front_index() const;
    const mem_slots_type &get_front_mem_slots() const;
    std::mutex &get_back_lock();
    cut_type &get_back_cut();
    uint8_t *get_back_index();
    mem_slots_type &get_back_mem_slots();

    float get_deliver_time() const;

private:
    size_t _size_index;
    size_t _size_mem_x;
    size_t _size_mem_y;
    size_t _size_feedback;

    std::mutex _front_lock;
    cut_type _front_cut;
    uint8_t *_front_index;
    mem_slots_type _front_mem_slots;

    std::mutex _back_lock;
    cut_type _back_cut;
    uint8_t *_back_index;
    mem_slots_type _back_mem_slots;

    float _deliver_time;
    bool _new_data;

    void deliver();
};
}

#endif // LAMURE_CUT_H
