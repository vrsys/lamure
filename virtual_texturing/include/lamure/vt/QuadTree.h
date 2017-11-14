//
// Created by sebastian on 05.11.17.
//

#ifndef QUAD_TREE_H
#define QUAD_TREE_H


#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <limits>
#include <math.h>
#include <memory>
#include <map>
#include <set>
#include <unordered_set>
#include <cstdint>
#include <iostream>

using namespace std;

namespace vt {

    class QuadTree {
    public:
        static const uint64_t get_child_id(uint64_t node_id, uint64_t child_index);

        static const uint64_t get_parent_id(uint64_t node_id);

        static const uint64_t get_first_node_id_of_depth(uint32_t depth);

        static const uint32_t get_length_of_depth(uint32_t depth);

        static const uint32_t get_depth_of_node(uint64_t node_id);

        static const uint32_t calculate_depth(size_t dim, size_t tile_size);

        static size_t get_tiles_per_row(uint32_t _depth);

    };

}


#endif //QUAD_TREE_H