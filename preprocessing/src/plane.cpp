// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/plane.h>

#include <iostream>

namespace lamure
{
namespace pre
{

plane_t::plane_t()
    : a_(0.0f), b_(0.0f), c_(0.0f), d_(0.0f), origin_(vec3r(0.f, 0.f, 0.f))
{

}

plane_t::plane_t(const vec3r &_normal, const vec3r &_origin)
    : origin_(_origin)
{
    vec3r n = vec3r(scm::math::normalize(_normal));
    a_ = n.x;
    b_ = n.y;
    c_ = n.z;
    d_ = -scm::math::dot(_origin, n);
}

vec3r plane_t::get_normal() const
{
    return vec3r(a_, b_, c_);
}

vec3r plane_t::get_origin() const
{
    return origin_;
}

real plane_t::signed_distance(const plane_t &_p, const vec3r &_v)
{
    return std::abs(_p.a_ * _v.x + _p.b_ * _v.y + _p.c_ * _v.z + _p.d_) / sqrt(_p.a_ * _p.a_ + _p.b_ * _p.b_ + _p.c_ * _p.c_);
}

vec3r plane_t::get_right() const
{
    vec3r normal = get_normal();
    vec3r up = vec3r(0.0f, 1.0f, 0.0f);
    vec3r right = vec3r(1.0f, 0.0f, 0.0f);

    if (normal != up) {
        right = scm::math::normalize(scm::math::cross(normal, up));
    }
    return right;
}

vec3r plane_t::get_up() const
{
    return scm::math::cross(get_normal(), get_right());
}

vec3r plane_t::get_point_on_plane(vec2r const &plane_coords) const
{
    return origin_
        + vec3r(get_right()) * plane_coords[0]
        + vec3r(get_up()) * plane_coords[1];
}

vec2r plane_t::project(const plane_t &_p, const vec3r &right, const vec3r &_v)
{
    real s = scm::math::dot(_v - _p.origin_, right);
    real t = scm::math::dot(_v - _p.origin_, _p.get_up());
    return vec2r(s, t);
}

vec2r plane_t::project(const plane_t &_p, const vec3r &right, const vec3r &up, const vec3r &_v)
{
    real s = scm::math::dot(_v - _p.origin_, right);
    real t = scm::math::dot(_v - _p.origin_, up);
    return vec2r(s, t);
}

void plane_t::fit_plane(
    std::vector<vec3r> const &neighbour_positions,
    plane_t &plane)
{

    vec3r centroid = vec3r{0.0};
    for (const auto &neighbour_pos : neighbour_positions) {
        centroid += neighbour_pos;
    }

    centroid /= (real) neighbour_positions.size();

    //calc covariance matrix
    mat3r covariance_mat = scm::math::mat3d::zero();

    for (const auto &c : neighbour_positions) {
        covariance_mat.m00 += (c[0] - centroid[0]) * (c[0] - centroid[0]);
        covariance_mat.m01 += (c[0] - centroid[0]) * (c[1] - centroid[1]);
        covariance_mat.m02 += (c[0] - centroid[0]) * (c[2] - centroid[2]);

        covariance_mat.m03 += (c[1] - centroid[1]) * (c[0] - centroid[0]);
        covariance_mat.m04 += (c[1] - centroid[1]) * (c[1] - centroid[1]);
        covariance_mat.m05 += (c[1] - centroid[1]) * (c[2] - centroid[2]);

        covariance_mat.m06 += (c[2] - centroid[2]) * (c[0] - centroid[0]);
        covariance_mat.m07 += (c[2] - centroid[2]) * (c[1] - centroid[1]);
        covariance_mat.m08 += (c[2] - centroid[2]) * (c[2] - centroid[2]);
    }

    //calculate normal
    mat3r inv_covariance_mat = scm::math::inverse(covariance_mat);

    vec3r first = vec3r{1.0f};
    vec3r second = scm::math::normalize(first * inv_covariance_mat);

    unsigned int iteration = 0;
    while (iteration++ < 512 && first != second) {
        first = second;
        second = scm::math::normalize(first * inv_covariance_mat);
    }

    plane = plane_t{second, centroid};
}

}

}