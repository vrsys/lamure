//
// Created by sebastian on 05.11.17.
//

#include <lamure/vt/QuadTree.h>

using namespace std;

namespace vt {

    const uint64_t QuadTree::get_child_id(const uint64_t node_id, const uint64_t child_index) {
        return node_id * 4 + 1 + child_index;
    }

    const uint64_t QuadTree::get_parent_id(const uint64_t node_id) {
        if (node_id == 0) return 0;

        if (node_id % 4 == 0) {
            return node_id / 4 - 1;
        } else {
            return (node_id + 4 - (node_id % 4)) / 4 - 1;
        }
    }

    const uint64_t QuadTree::get_first_node_id_of_depth(uint32_t depth) {
        uint64_t id = 0;
        for (uint32_t i = 0; i < depth; ++i) {
            id += (uint64_t) pow((double) 4, (double) i);
        }

        return id;
    }

    const uint32_t QuadTree::get_length_of_depth(uint32_t depth) {
        return (const uint32_t) pow((double) 4, (double) depth);
    }

    const uint32_t QuadTree::get_depth_of_node(const uint64_t node_id) {
        return (uint32_t) (log((node_id + 1) * (4 - 1)) / log(4));
    }

    const uint16_t QuadTree::calculate_depth(size_t dim, size_t tile_size) {
        size_t dim_tiled = dim / tile_size;
        return (uint16_t) (log(dim_tiled * dim_tiled) / log(4));
    }

    size_t QuadTree::get_tiles_per_row(uint32_t _depth) {
        return (size_t) pow(2, _depth);
    }

}
