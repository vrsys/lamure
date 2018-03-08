//
// Created by sebastian on 13.11.17.
//

#ifndef VT_COMMON_H
#define VT_COMMON_H

#include <lamure/config.h>
#include <cstdint>
#include <cstddef>
#include <set>
#include <vector>
#include <map>
namespace vt
{
typedef uint64_t id_type;
typedef uint32_t priority_type;
typedef std::set<id_type> cut_type;

struct mem_slot_type
{
    size_t position = SIZE_MAX;
    id_type tile_id = UINT64_MAX;
    uint8_t *pointer = nullptr;
    bool locked = false;
    bool updated = false;
};

typedef std::vector<mem_slot_type> mem_slots_type;
typedef std::map<id_type, size_t> mem_slots_index_type;
class Cut;
typedef std::map<uint64_t, Cut*> cut_map_type;
typedef std::pair<uint64_t, Cut*> cut_map_entry_type;
}

#endif // VT_COMMON_H
