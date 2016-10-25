
#ifndef LAMURE_MESH_TYPES_H_
#define LAMURE_MESH_TYPES_H_

#include <lamure/types.h>

#include <climits>

namespace lamure {
namespace mesh {

enum axis_t {
  AXIS_X = 0,
  AXIS_Y = 1,
  AXIS_Z = 2
};

struct indexed_vertex_t {
  uint32_t id_;
  vec3r_t v_;

  indexed_vertex_t()
  : id_(0), v_(0.0) {};

  indexed_vertex_t(const uint32_t _id, const vec3r_t& _v)
  : id_(_id), v_(_v) {};

  bool operator == (const indexed_vertex_t& _rhs) const {
    return v_ == _rhs.v_;
  }
};


struct indexed_edge_t {
  uint32_t id_;
  indexed_vertex_t v0_;
  indexed_vertex_t v1_;
  float32_t cost_;

  indexed_edge_t()
  : id_(0), v0_(), v1_(), cost_(std::numeric_limits<float32_t>::max()) {}; 

  indexed_edge_t(const uint32_t _id, const indexed_vertex_t& _v0, const indexed_vertex_t& _v1)
  : id_(_id), v0_(_v0), v1_(_v1), cost_(std::numeric_limits<float32_t>::max()) {}; 

  bool operator == (const indexed_edge_t& _e) const {
    return (v0_.v_ == _e.v0_.v_ && v1_.v_ == _e.v1_.v_) || (v1_.v_ == _e.v0_.v_ && v0_.v_ == _e.v1_.v_);
  }   

  bool is_connected_edge(const indexed_edge_t& _e) const {
    return v0_.v_ == _e.v1_.v_ || v1_.v_ == _e.v1_.v_ || v1_.v_ == _e.v0_.v_ || v0_.v_ == _e.v1_.v_;
  }   
};


}
}

#endif
