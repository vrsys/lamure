// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef COMMON_TYPES_H_
#define COMMON_TYPES_H_

#include <scm/core/math/math.h>

namespace lamure {

// default type for storing coordinates
using real = double; //< for surfel position and radius

// tree types
using NodeIdType = uint32_t;

// math
using vec3r  = scm::math::vec<real, 3>; //< for surfel position
using vec3f  = scm::math::vec3f;
using vec3ui = scm::math::vec<uint32_t, 3>;
using vec3b  = scm::math::vec<uint8_t, 3>;
using mat4r  = scm::math::mat<real, 4, 4>;
using mat4f  = scm::math::mat4f;

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

#endif // COMMON_TYPES_H_

