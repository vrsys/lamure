
#ifndef LAMURE_MESH_BVH_H_
#define LAMURE_MESH_BVH_H_

#include <lamure/types.h>
#include <lamure/mesh/triangle.h>

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

protected:

  void create_hierarchy(std::vector<triangle_t>& triangles);

  int32_t depth_;
  std::vector<bvh_node> nodes_;

};

}
}

#endif