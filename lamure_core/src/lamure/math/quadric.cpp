// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/math/quadric.h>

namespace lamure {
namespace math {

quadric_t::quadric_t()
    : a_(0.f), b_(0.f), c_(0.f), d_(0.f),
      e_(0.f), f_(0.f), g_(0.f),
      h_(0.f), i_(0.f),
      j_(0.f) {};

quadric_t::quadric_t(float32_t _a, float32_t _b, float32_t _c, float32_t _d)
    : a_(_a*_a), b_(_a*_b), c_(_a*_c), d_(_a*_d),
      e_(_b*_b), f_(_b*_c), g_(_b*_d),
      h_(_c*_c), i_(_c*_d),
      j_(_d*_d) {};

quadric_t::quadric_t(const plane_t& _p)
    : a_(_p.a_*_p.a_), b_(_p.a_*_p.b_), c_(_p.a_*_p.c_), d_(_p.a_*_p.d_),
      e_(_p.b_*_p.b_), f_(_p.b_*_p.c_), g_(_p.b_*_p.d_),
      h_(_p.c_*_p.c_), i_(_p.c_*_p.d_),
      j_(_p.d_*_p.d_) {
}

quadric_t::quadric_t(const quadric_t& _q)
    : a_(_q.a_), b_(_q.b_), c_(_q.c_), d_(_q.d_),
      e_(_q.e_), f_(_q.f_), g_(_q.g_),
      h_(_q.h_), i_(_q.i_),
      j_(_q.j_) {

}

quadric_t::~quadric_t() {

}

quadric_t quadric_t::operator *(const float32_t _s) const {
    quadric_t q;
    q.a_ = a_*_s;
    q.b_ = b_*_s;
    q.c_ = c_*_s;
    q.d_ = d_*_s;
    q.e_ = e_*_s;
    q.f_ = f_*_s;
    q.g_ = g_*_s;
    q.h_ = h_*_s;
    q.i_ = i_*_s;
    q.j_ = j_*_s;
    return q;
}

quadric_t quadric_t::operator +(const quadric_t _q) const {
    quadric_t q;
    q.a_ = a_+_q.a_;
    q.b_ = b_+_q.b_;
    q.c_ = c_+_q.c_;
    q.d_ = d_+_q.d_;
    q.e_ = e_+_q.e_;
    q.f_ = f_+_q.f_;
    q.g_ = g_+_q.g_;
    q.h_ = h_+_q.h_;
    q.i_ = i_+_q.i_;
    q.j_ = j_+_q.j_;
    return q;
}


quadric_t& quadric_t::operator *=(const float32_t _s) {
    a_ *= _s;
    b_ *= _s;
    c_ *= _s;
    d_ *= _s;
    e_ *= _s;
    f_ *= _s;
    g_ *= _s;
    h_ *= _s;
    i_ *= _s;
    j_ *= _s;
    return *this;
}

quadric_t& quadric_t::operator +=(const quadric_t _q) {
    a_ += _q.a_;
    b_ += _q.b_;
    c_ += _q.c_;
    d_ += _q.d_;
    e_ += _q.e_;
    f_ += _q.f_;
    g_ += _q.g_;
    h_ += _q.h_;
    i_ += _q.i_;
    j_ += _q.j_;
    return *this;
}


float32_t quadric_t::vqv(const quadric_t& _q, const vec3f_t& _v) {
    return _q.a_*_v.x_*_v.x_ + 2.f*_q.b_*_v.x_*_v.y_ + 2.f*_q.c_*_v.x_*_v.z_ + 2.f*_q.d_*_v.x_
         + _q.e_*_v.y_*_v.y_ + 2.f*_q.f_*_v.y_*_v.z_ + 2.f*_q.g_*_v.y_
         + _q.h_*_v.z_*_v.z_ + 2.f*_q.i_*_v.z_
         + _q.j_;
}

}
}

