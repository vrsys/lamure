#ifndef KDTREE_H_
#define KDTREE_H_

#include <scm/core/math.h>
#include <lamure/mesh/triangle.h>

#include <iostream>
#include <vector>
#include <climits>
#include <queue>

class indexed_triangle_t : public lamure::mesh::triangle_t
{
  public:
    uint32_t tex_idx_;
    uint32_t tri_id_;
};

class kdtree_t
{
  public:
    struct node_t
    {
        uint32_t node_id_;
        scm::math::vec3f min_;
        scm::math::vec3f max_;
        uint32_t begin_;
        uint32_t end_;
        float error_;
    };

    enum axis_t
    {
        AXIS_X = 0,
        AXIS_Y = 1,
        AXIS_Z = 2
    };

    kdtree_t(const std::vector<indexed_triangle_t>& _triangles, uint32_t _tris_per_node);

    uint32_t get_num_triangles_per_node() const { return num_triangles_per_node_; };
    const std::vector<node_t>& get_nodes() const { return nodes_; };
    const std::vector<uint32_t>& get_indices() const { return indices_; };
    uint32_t get_depth() const { return depth_; };

    static uint32_t invalid_node_id_t;

    const uint32_t get_child_id(const uint32_t node_id, const uint32_t child_index) const;
    const uint32_t get_parent_id(const uint32_t node_id) const;
    const uint32_t get_depth_of_node(const uint32_t node_id) const;
    const uint32_t get_length_of_depth(const uint32_t depth) const;
    const uint32_t get_first_node_id_of_depth(const uint32_t depth) const;

  protected:
    void create(const std::vector<indexed_triangle_t>& _triangles);
    void sort_indices(const kdtree_t::node_t& _node, const std::vector<indexed_triangle_t>& _triangles, std::vector<uint32_t>& indices, kdtree_t::axis_t _axis);

  private:
    uint32_t depth_;
    uint32_t num_triangles_per_node_;
    uint32_t fanout_factor_;
    std::vector<node_t> nodes_;
    std::vector<uint32_t> indices_;
};

#endif
