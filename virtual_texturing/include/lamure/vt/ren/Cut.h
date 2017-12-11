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

    const std::set<id_type> &get_front_cut() const;
    const uint8_t *get_front_index() const;
    const std::queue<std::pair<size_t, uint8_t *>> &get_front_mem_cut() const;
    const id_type *get_front_mem_slots() const;

    std::set<id_type> &get_back_cut();
    uint8_t *get_back_index();
    std::queue<std::pair<size_t, uint8_t *>> &get_back_mem_cut();
    id_type *get_back_mem_slots();
    std::set<size_t> &get_back_mem_slots_free();

  private:
    size_t _size_index;
    size_t _size_mem_x;
    size_t _size_mem_y;
    size_t _size_feedback;

    std::mutex _front_lock;
    std::set<id_type> _front_cut;
    uint8_t *_front_index;
    std::queue<std::pair<size_t, uint8_t *>> _front_mem_cut;
    id_type *_front_mem_slots;
    std::set<size_t> _front_mem_slots_free;

    std::mutex _back_lock;
    std::set<id_type> _back_cut;
    uint8_t *_back_index;
    std::queue<std::pair<size_t, uint8_t *>> _back_mem_cut;
    id_type *_back_mem_slots;
    std::set<size_t> _back_mem_slots_free;
    bool _new_data;

    void _swap();
};
}

#endif // LAMURE_CUT_H
