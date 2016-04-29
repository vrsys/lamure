// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_TYPES_H_
#define LAMURE_TYPES_H_

#include <stdint.h>
#include <lamure/math.h>

typedef float float32_t;
typedef double float64_t;

namespace lamure {

// default type for storing coordinates
using real_t = double; //< for surfel position and radius

// tree node index types
using node_id_t = uint32_t;

// global identifier of surfel
struct surfel_id_t {
    node_id_t node_idx;
    size_t surfel_idx;

    surfel_id_t(node_id_t node_i = -1, size_t surfel_i = -1)
     :node_idx(node_i)
     ,surfel_idx(surfel_i)
    {}

    friend bool operator==(surfel_id_t const& lhs, surfel_id_t const& rhs) {
        return lhs.node_idx == rhs.node_idx &&
               lhs.surfel_idx == rhs.surfel_idx;
    }
 
    friend bool operator!=(surfel_id_t const& lhs, surfel_id_t const& rhs) {
        return lhs.node_idx != rhs.node_idx ||
               lhs.surfel_idx != rhs.surfel_idx;
    }

    friend bool operator<(surfel_id_t const& lhs, surfel_id_t const& rhs) {
      if(lhs.node_idx < rhs.node_idx) {
        return true;
      }
      else {
        if(lhs.node_idx == rhs.node_idx) {
          return lhs.surfel_idx < rhs.surfel_idx;
        }
        else return false;
      }
    }

    friend std::ostream& operator<<(std::ostream& os, const surfel_id_t& s) {
      os << "(" << s.node_idx << "," << s.surfel_idx << ")";
      return os;
    }
};

// math
using vec2r_t  = lamure::math::vector_t<real_t, 2>;
using vec2f_t  = lamure::math::vec2f_t;
using vec2d_t  = lamure::math::vec2d_t;
using vec3r_t  = lamure::math::vector_t<real_t, 3>; //< for surfel position
using vec3f_t  = lamure::math::vec3f_t;
using vec3d_t  = lamure::math::vec3d_t;
using vec3ui_t = lamure::math::vector_t<uint32_t, 3>;
using vec3b_t  = lamure::math::vector_t<uint8_t, 3>;
using vec4r_t  = lamure::math::vector_t<real_t, 4>; //< for surfel position
using vec4f_t  = lamure::math::vec4f_t;
using vec4d_t  = lamure::math::vec4d_t;
using mat3r_t  = lamure::math::matrix_t<real_t, 3, 3>;
using mat3f_t  = lamure::math::mat3f_t;
using mat3d_t  = lamure::math::mat3d_t;
using mat4r_t  = lamure::math::matrix_t<real_t, 4, 4>;
using mat4f_t  = lamure::math::mat4f_t;
using mat4d_t  = lamure::math::mat4d_t;

//rendering system types
using node_t    = size_t;
using slot_t    = size_t;
using model_t   = uint32_t;
using view_t    = uint32_t;
using context_t = uint32_t;

//rendering system invalid types
const static node_t invalid_node_t       = std::numeric_limits<node_t>::max();
const static slot_t invalid_slot_t       = std::numeric_limits<slot_t>::max();
const static model_t invalid_model_t     = std::numeric_limits<model_t>::max();
const static view_t invalid_view_t       = std::numeric_limits<view_t>::max();
const static context_t invalid_context_t = std::numeric_limits<context_t>::max();



} // namespace lamure

#endif // LAMURE_TYPES_H_

