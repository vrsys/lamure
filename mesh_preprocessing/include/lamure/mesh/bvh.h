
#ifndef LAMURE_MESH_BVH_H_
#define LAMURE_MESH_BVH_H_

#include <lamure/types.h>
#include <lamure/mesh/triangle.h>
//#include <lamure/ren/bvh.h>

namespace lamure {
namespace mesh {

struct bvh_node {
  int32_t depth_; //depth of the node in the hierarchy
  vec3f min_;
  vec3f max_;
  int64_t begin_; //first triangle id that belongs to this node
  int64_t end_; //last triangle
};

class bvh {
public:
  bvh(std::vector<triangle_t>& triangles);
  ~bvh();


  const uint32_t get_child_id(const uint32_t node_id, const uint32_t child_index) const;
  const uint32_t get_parent_id(const uint32_t node_id) const;
  const uint32_t get_first_node_id_of_depth(uint32_t depth) const;
  const uint32_t get_length_of_depth(uint32_t depth) const;
  const uint32_t get_depth_of_node(const uint32_t node_id) const;

protected:

  void create_hierarchy(std::vector<triangle_t>& triangles);

  int32_t depth_;
  std::vector<bvh_node> nodes_;

};

}
}

#endif