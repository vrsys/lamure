// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_CORE_MATH_QUADRIC_H_
#define LAMURE_CORE_MATH_QUADRIC_H_

#include <lamure/math.h>
#include <lamure/math/plane.h>
#include <lamure/types.h>
#include <lamure/platform_core.h>

namespace lamure {
namespace math {

class quadric_t {
public:
    quadric_t();
    quadric_t(float32_t _a, float32_t _b, float32_t _c, float32_t _d);
    quadric_t(const plane_t& _p);
    quadric_t(const quadric_t& _q);
    ~quadric_t();

    quadric_t operator *(const float32_t _s) const;
    quadric_t operator +(const quadric_t _rq) const;
    quadric_t& operator *=(const float32_t _s);
    quadric_t& operator +=(const quadric_t _q);

    static float32_t vqv(const quadric_t& _q, const vec3f_t& _v);

private:
    float32_t a_, b_, c_, d_;
    float32_t e_, f_, g_;
    float32_t h_, i_;
    float32_t j_;

};


} // namespace math
} // namespace lamure

#endif

