
#ifndef LMR_MRP_CHART_HEAP_H_INCLUDED
#define LMR_MRP_CHART_HEAP_H_INCLUDED

#include <lamure/types.h>

#include <mutex>
#include <vector>
#include <map>


namespace lamure {
namespace util {

typedef uint32_t slot_id_t;

template<typename item_t>
class heap_base_t {
public:
    heap_base_t();
    virtual ~heap_base_t();

    const slot_id_t     get_num_slots();
    const bool          is_item_indexed(const uint32_t _item_id);

    void                push(const item_t& _item);
    const item_t        top();
    void                pop();

    void                remove_all(const uint32_t _item_id,
                                   std::vector<item_t>& _removed);

protected:
    void                shuffle_up(const slot_id_t _slot_id);
    void                shuffle_down(const slot_id_t _slot_id);

    void                swap(const slot_id_t _slot_id_0, const slot_id_t _slot_id_1);

    slot_id_t           num_slots_;
    std::mutex          mutex_;

    std::vector<item_t> slots_;

    //mapping item_id to slot
    std::map<uint32_t, slot_id_t> slot_map_;

};

} // namespace util
} // namespace lamure

#include <lamure/util/heap.inl>

#endif
