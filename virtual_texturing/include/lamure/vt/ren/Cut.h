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

    const cut_type &get_front_cut() const;
    const uint8_t *get_front_index() const;
    const mem_cut_type &get_front_mem_cut() const;
    const id_type *get_front_mem_slots() const;
    const mem_slots_free_type &get_front_mem_slots_free() const;
    const float &get_swap_time() const;

    cut_type &get_back_cut();
    uint8_t *get_back_index();
    mem_cut_type &get_back_mem_cut();
    id_type *get_back_mem_slots();
    mem_slots_free_type &get_back_mem_slots_free();

  private:
    size_t _size_index;
    size_t _size_mem_x;
    size_t _size_mem_y;
    size_t _size_feedback;

    std::mutex _front_lock;
    cut_type _front_cut;
    uint8_t *_front_index;
    mem_cut_type _front_mem_cut;
    id_type *_front_mem_slots;
    mem_slots_free_type _front_mem_slots_free;

    std::mutex _back_lock;
    cut_type _back_cut;
    uint8_t *_back_index;
    mem_cut_type _back_mem_cut;
    id_type *_back_mem_slots;
    mem_slots_free_type _back_mem_slots_free;
    bool _new_data;

    float _swap_time;

    void _swap();
};
}

#endif // LAMURE_CUT_H
