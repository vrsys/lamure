// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PLANE_H_INCLUDED
#define PLANE_H_INCLUDED

#include <scm/core/math.h>
#include <complex>

class plane_t {
public:
    plane_t();
    plane_t(const scm::math::vec3f& _normal, const scm::math::vec3f& _origin);
    plane_t(float _a, float _b, float _c, float _d);
    plane_t(const plane_t& _p);
    ~plane_t();

    scm::math::vec3f get_normal() const;
    scm::math::vec3f get_origin() const;
    scm::math::vec3f get_right() const;

    static float signed_distance(const plane_t& _p, const scm::math::vec3f& _v);
    static scm::math::vec2f project(const plane_t& _p, const scm::math::vec3f& _right, const scm::math::vec3f& _v);

    float a_;
    float b_;
    float c_;
    float d_;

    scm::math::vec3f origin_;
};

#endif
