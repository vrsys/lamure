// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_CORE_MATH_PLANE_H_
#define LAMURE_CORE_MATH_PLANE_H_

#include <lamure/types.h>
#include <lamure/math.h>
#include <lamure/platform_core.h>

#include <complex>
#include <vector>

namespace lamure {
namespace math {

class CORE_DLL plane_t {
public:
    plane_t();
    plane_t(const vec3r_t& _normal, const vec3r_t& _origin);

    enum classification_result_t {
      FRONT,
      BACK,
      INSIDE
    };

    vec3r_t get_normal() const;
    vec3r_t get_origin() const;
    vec3r_t get_right() const;
    vec3r_t get_up() const;

    vec3r_t get_point_on_plane(vec2r_t const& plane_coords) const;

    void transform(const mat4r_t& _t);
    classification_result_t classify(const vec3r_t& _p) const;

    static real_t signed_distance(const plane_t& _p, const vec3r_t& _v);
    static vec2r_t project(const plane_t& _p, const vec3r_t& _right, const vec3r_t& _v);
    static vec2r_t project(const plane_t& _p, const vec3r_t& right, const vec3r_t& up, const vec3r_t& _v);

    static void fit_plane(
    std::vector<vec3r_t> const& neighbour_pos_ptrs, plane_t& plane);

    real_t a_;
    real_t b_;
    real_t c_;
    real_t d_;

    vec3r_t origin_;
};

} // namespace math
} // namespace lamure

#endif
