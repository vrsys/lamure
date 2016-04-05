// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PLANE_H_INCLUDED
#define PLANE_H_INCLUDED

#include <lamure/types.h>

#include <scm/core/math.h>
#include <complex>


namespace lamure {
namespace pre {

class plane_t {
public:
    plane_t();
    plane_t(const vec3r& _normal, const vec3r& _origin);
    plane_t(float _a, float _b, float _c, float _d);
    plane_t(const plane_t& _p);
    ~plane_t();

    scm::math::vec3f get_normal() const;
    vec3r get_origin() const;
    scm::math::vec3f get_right() const;
    scm::math::vec3f get_up() const;

    vec3r get_point_on_plane( vec2r const& plane_coords) const;

    static float signed_distance(const plane_t& _p, const scm::math::vec3f& _v);
    static vec2r project(const plane_t& _p, const scm::math::vec3f& _right, const vec3r& _v);

    static void fit_plane(
    std::vector<vec3r> const& neighbour_pos_ptrs,
    plane_t& plane);

    float a_;
    float b_;
    float c_;
    float d_;

    vec3r origin_;
};

}
}

#endif
