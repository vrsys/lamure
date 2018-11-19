
#ifndef LAMURE_MESH_BVH_H_
#define LAMURE_MESH_BVH_H_

#include <lamure/types.h>
#include <lamure/mesh/triangle.h>
#include <lamure/ren/bvh.h>

#include <map>
#include <vector>

namespace lamure {
namespace mesh {


class bvh : public lamure::ren::bvh {
public:
  bvh(std::vector<triangle_t>& triangles, uint32_t primitives_per_node);
  ~bvh();

  void write_lod_file(const std::string& lod_filename);

protected:

  
  struct bvh_node {
    uint32_t depth_; //depth of the node in the hierarchy
    vec3f min_;
    vec3f max_;
    uint64_t begin_; //first triangle id that belongs to this node
    uint64_t end_; //last triangle
  };

  void create_hierarchy(std::vector<triangle_t>& triangles);
  

  void simplify(
    std::vector<triangle_t>& left_child_tris,
    std::vector<triangle_t>& right_child_tris,
    std::vector<triangle_t>& output_tris);


  //node_id -> triangles
  std::map<uint32_t, std::vector<triangle_t>> triangles_map_;

};

}
}

#endif