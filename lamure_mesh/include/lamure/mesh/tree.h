
#ifndef LAMURE_MESH_TREE_H_
#define LAMURE_MESH_TREE_H_

#include <lamure/types.h>
#include <lamure/mesh/types.h>
#include <lamure/mesh/triangle.h>

#include <vector>
#include <iostream>
#include <cstring>

namespace lamure {
namespace mesh {

struct tree_node_t {
  node_t node_id_;
  vec3r_t min_;
  vec3r_t max_;
  uint32_t begin_;
  uint32_t end_;
  //uint32_t depth_; //no longer necessary
  float32_t error_;
  float32_t max_blend_;
  float32_t unused_;
};

class tree_t {
public:
    ~tree_t();

    const node_t get_child_id(const node_t node_id, const uint32_t child_index) const;
    const node_t get_parent_id(const node_t node_id) const;
    const uint32_t get_depth_of_node(const node_t node_id) const;
    const uint32_t get_length_of_depth(uint32_t depth) const;
    const node_t get_first_node_id_of_depth(const uint32_t depth) const;

    const uint32_t get_num_nodes() const { return num_nodes_; };
    const std::vector<tree_node_t>& get_nodes() const { return nodes_; };
    std::vector<tree_node_t>& get_nodes_unsafe() { return nodes_; };
    const uint32_t get_fanout_factor() const { return fanout_factor_; };
    const uint32_t get_depth() const { return depth_; };

protected:
    tree_t(uint32_t fanout_factor);

    void set_fanout_factor(const uint32_t fanout_factor) { fanout_factor_ = fanout_factor; };
    void set_depth(const uint32_t depth) { depth_ = depth; };
    void set_num_nodes(const uint32_t num_nodes) { num_nodes_ = num_nodes; };
    void set_nodes(const std::vector<tree_node_t>& nodes) { nodes_ = nodes; };

protected:
    uint32_t fanout_factor_;
    uint32_t depth_;
    uint32_t num_nodes_;
    std::vector<tree_node_t> nodes_;

    void sort_indices(const tree_node_t& node, const triangle_t* triangles, uint32_t* indices, axis_t axis);

};

}
}


#endif
