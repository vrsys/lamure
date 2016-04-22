// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_MATH_BOUNDING_SPHERE_H_
#define LAMURE_MATH_BOUNDING_SPHERE_H_

#include <lamure/platform_core.h>
#include <lamure/types.h>

namespace lamure {
namespace math {

class bounding_box_t;

class CORE_DLL bounding_sphere_t
{
public:
                        bounding_sphere_t(const vec3r_t center = vec3r_t(0.0),
                               const real_t radius = 0.0)
                               : center_(center), radius_(radius)
                            {

                            }

    virtual             ~bounding_sphere_t() {}

    inline const vec3r_t  center() const { return center_; }
    inline const real_t  radius() const { return radius_; }

    inline bool         operator==(const bounding_sphere_t& rhs) const
                            { return center_ == rhs.center_ && radius_ == rhs.radius_; }
    inline bool         operator!=(const bounding_sphere_t& rhs) const
                            { return !(operator==(rhs)); }

    const bounding_box_t   get_bounding_box() const;

    bool		        contains(const vec3r_t& point) const;

    real_t                clamp_to_AABB_face(real_t actual_value, 
                                           real_t min_BB_value, real_t max_BB_value) const;

    vec3r_t               get_closest_point_on_AABB(const bounding_box_t& bounding_box) const;

    bool		        intersects_or_contains(const bounding_box_t& bounding_box) const;

private:

    vec3r_t               center_;
    real_t                radius_;

};

} // namespace math
} // namespace lamure

#endif // LAMURE_MATH_BOUNDING_SPHERE_H_
