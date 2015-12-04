// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "plane.h"

plane_t::plane_t()
    : a_(0.0f), b_(0.0f), c_(0.0f), d_(0.0f), origin_(scm::math::vec3f(0.f, 0.f, 0.f)) {

}

plane_t::plane_t(const scm::math::vec3f& _normal, const scm::math::vec3f& _origin)
    : origin_(_origin) {
    scm::math::vec3f n = scm::math::normalize(_normal);
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

scm::math::vec2f plane_t::project(const plane_t& _p, const scm::math::vec3f& _right, const scm::math::vec3f& _v) {
    scm::math::vec3f r = scm::math::normalize(_right);
    float s = scm::math::dot(_v - _p.origin_, r);
    float t = scm::math::dot(_v - _p.origin_, scm::math::cross(_p.get_normal(), r));
    return scm::math::vec2f(s, t);
}

