
#ifndef LAMURE_MESH_TRIANGLE_H_
#define LAMURE_MESH_TRIANGLE_H_

#include <lamure/types.h>
#include <lamure/math/triangle.h>

namespace lamure {
namespace mesh {

struct vertex_data_serialized_t {
  vec3r_t va_;
  vec3r_t vb_;
  vec3r_t vc_;
  vec3r_t na_;
  vec3r_t nb_;
  vec3r_t nc_;
  vec2r_t ca_;
  vec2r_t cb_;
  vec2r_t cc_;

};

typedef lamure::math::triangle_base_t<vertex_data_serialized_t> triangle_t;


}
}


#endif
