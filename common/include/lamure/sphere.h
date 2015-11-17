// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef COMMON_SPHERE_H_
#define COMMON_SPHERE_H_

#include <lamure/platform.h>
#include <lamure/types.h>

namespace lamure
{

class BoundingBox;

class COMMON_DLL Sphere
{
public:
                        Sphere(const vec3r center = vec3r(0.0),
                               const real radius = 0.0)
                               : center_(center), radius_(radius)
                            {

                            }

    virtual             ~Sphere() {}

    inline const vec3r  center() const { return center_; }
    inline const real  radius() const { return radius_; }

    inline bool         operator==(const Sphere& rhs) const
                            { return center_ == rhs.center_ && radius_ == rhs.radius_; }
    inline bool         operator!=(const Sphere& rhs) const
                            { return !(operator==(rhs)); }

    const BoundingBox   GetBoundingBox() const;

    bool		        Contains(const vec3r& point) const;
    bool		        IsInside(const BoundingBox& bounding_box) const;

private:

    vec3r               center_;
    real                radius_;

};

} // namespace lamure

#endif // COMMON_SPHERE_H_
