
#ifndef LAMURE_MESH_BVH_H_
#define LAMURE_MESH_BVH_H_

#include <lamure/types.h>
#include <lamure/mesh/triangle.h>
//#include <lamure/ren/bvh.h>

#include <map>
#include <vector>

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
  

  void simplify(
    std::vector<triangle_t>& left_child_tris,
    std::vector<triangle_t>& right_child_tris,
    std::vector<triangle_t>& output_tris);

  int32_t depth_;
  std::vector<bvh_node> nodes_;

  //node_id -> triangles
  std::map<uint32_t, std::vector<triangle_t>> triangles_map_;

};

}
}

#endif