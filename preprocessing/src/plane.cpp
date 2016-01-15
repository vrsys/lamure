// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/plane.h>

#include <iostream>

namespace lamure {
namespace pre {

plane_t::plane_t()
    : a_(0.0f), b_(0.0f), c_(0.0f), d_(0.0f), origin_(vec3r(0.f, 0.f, 0.f)) {

}

plane_t::plane_t(const vec3r& _normal, const vec3r& _origin)
    : origin_(_origin) {
    vec3r n = vec3r(scm::math::normalize(_normal));
    a_ = n.x;
    b_ = n.y;
    c_ = n.z;
    d_ = -scm::math::dot(_origin, n);
}

plane_t::plane_t(float _a, float _b, float _c, float _d) 
    : a_(_a), b_(_b), c_(_c), d_(_d) {

}

plane_t::plane_t(const plane_t& _p)
    : a_(_p.a_), b_(_p.b_), c_(_p.c_), d_(_p.d_) {
}

plane_t::~plane_t() {

}

scm::math::vec3f plane_t::get_normal() const {
    return scm::math::vec3f(a_, b_, c_);
}

scm::math::vec3f plane_t::get_origin() const {
    return origin_;
}

float plane_t::signed_distance(const plane_t& _p, const scm::math::vec3f& _v) {
    return std::abs(_p.a_ * _v.x + _p.b_ * _v.y + _p.c_ * _v.z + _p.d_) / sqrt(_p.a_*_p.a_ + _p.b_*_p.b_ + _p.c_*_p.c_);
}

scm::math::vec3f plane_t::get_right() const {
    scm::math::vec3f normal = scm::math::normalize(get_normal());
    scm::math::vec3f up = scm::math::vec3f(0.0f, 1.0f, 0.0f);
    scm::math::vec3f right = scm::math::vec3f(1.0f, 0.0f, 0.0f);
    if (normal != up) {
        right = scm::math::normalize(scm::math::cross(normal, up));
    }
    return right;
}

scm::math::vec3f plane_t::get_up() const {
    return scm::math::cross( get_normal(), get_right() );
}

vec2r plane_t::project(const plane_t& _p, const scm::math::vec3f& _right, const vec3r& _v) {
    vec3r r = vec3r( scm::math::normalize(_right) );
    real s = scm::math::dot(_v - _p.origin_, r);
    real t = scm::math::dot(_v - _p.origin_, vec3r(_p.get_up()) );

    return vec2r(s, t);
}


void plane_t::fit_plane(
    std::vector<vec3r>& neighbour_pos_ptrs,
    plane_t& plane) {
    
    unsigned int num_neighbours = neighbour_pos_ptrs.size();

    vec3r cen = vec3r(0.0, 0.0, 0.0);

    for (const auto& pos_ptr : neighbour_pos_ptrs) {
        vec3r neighbour_pos = pos_ptr;
        cen.x += neighbour_pos[0];
        cen.y += neighbour_pos[1];
        cen.z += neighbour_pos[2];
    }
    
    vec3r centroid = cen * (1.0f/ (real)num_neighbours);

    //calc covariance matrix
    mat3r covariance_mat = scm::math::mat3d::zero();

    for (const auto& pos_ptr : neighbour_pos_ptrs) {
        vec3r const& c = pos_ptr;

        covariance_mat.m00 += std::pow(c[0]-centroid[0], 2);
        covariance_mat.m01 += (c[0]-centroid[0]) * (c[1] - centroid[1]);
        covariance_mat.m02 += (c[0]-centroid[0]) * (c[2] - centroid[2]);

        covariance_mat.m03 += (c[1]-centroid[1]) * (c[0] - centroid[0]);
        covariance_mat.m04 += std::pow(c[1]-centroid[1], 2);
        covariance_mat.m05 += (c[1]-centroid[1]) * (c[2] - centroid[2]);

        covariance_mat.m06 += (c[2]-centroid[2]) * (c[0] - centroid[0]);
        covariance_mat.m07 += (c[2]-centroid[2]) * (c[1] - centroid[1]);
        covariance_mat.m08 += std::pow(c[2]-centroid[2], 2);

    }

    //calculate normal
    mat3r inv_covariance_mat = scm::math::inverse(covariance_mat);

    vec3r first = vec3r(1.0f, 1.0f, 1.0f);
    vec3r second = scm::math::normalize(first*inv_covariance_mat);

    unsigned int iteration = 0;
    

    while (iteration++ < 512 && first != second) {
        first = second;
        second = scm::math::normalize(first*inv_covariance_mat);
    }
    
    plane = plane_t(second, centroid);
}

}

}