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
#include <vector>


namespace lamure {
namespace pre {

class plane_t {
public:
    plane_t();
    plane_t(const vec3r& _normal, const vec3r& _origin);

    vec3r get_normal() const;
    vec3r get_origin() const;
    vec3r get_right() const;
    vec3r get_up() const;

    vec3r get_point_on_plane( vec2r const& plane_coords) const;

    static real signed_distance(const plane_t& _p, const vec3r& _v);
    static vec2r project(const plane_t& _p, const vec3r& _right, const vec3r& _v);
    static vec2r project(const plane_t& _p, const vec3r& right, const vec3r& up, const vec3r& _v);

    static void fit_plane(
    std::vector<vec3r> const& neighbour_pos_ptrs,
    plane_t& plane);

    real a_;
    real b_;
    real c_;
    real d_;

    vec3r origin_;
};

}
}

#endif
