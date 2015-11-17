// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_SURFEL_H_
#define PRE_SURFEL_H_

#include <lamure/pre/platform.h>
#include <lamure/types.h>

#include <vector>
#include <functional>
#include <memory>

namespace lamure {
namespace pre
{

class PREPROCESSING_DLL Surfel /*final*/
{
public:
    using CompareFunction = std::function<bool(const Surfel& left, const Surfel& right)>;

                        Surfel(const vec3r& pos    = vec3r(0.0),
                               const vec3b& color  = vec3b(0),
                               const real   radius = 0.0,
                               const vec3f& normal = vec3f(0.f))
                            : pos_(pos),
                              color_(color),
                              radius_(radius),
                              normal_(normal) {}

    const vec3r         pos() const { return pos_; }
    vec3r&              pos() { return pos_; }

    const vec3b         color() const { return color_; }
    vec3b&              color() { return color_; }
    
    const real          radius() const { return radius_; }
    real&               radius() { return radius_; }

    const vec3f         normal() const { return normal_; }
    vec3f&              normal() { return normal_; }

    bool                operator==(const Surfel& rhs) const
                            { return pos_    == rhs.pos_ &&
                                     color_  == rhs.color_ &&
                                     radius_ == rhs.radius_ &&
                                     normal_ == rhs.normal_; }

    bool                operator!=(const Surfel& rhs) const
                            { return !(operator==(rhs)); }

    static bool         CompareX(const Surfel &left, const Surfel &right);
    static bool         CompareY(const Surfel &left, const Surfel &right);
    static bool         CompareZ(const Surfel &left, const Surfel &right);

    static CompareFunction
                        Compare(const uint8_t axis);

private:

    vec3r               pos_;
    vec3b               color_;
    real                radius_;
    vec3f               normal_;

};

using SurfelVector = std::vector<Surfel>;
using SharedSurfelVector = std::shared_ptr<SurfelVector>;

} } // namespace lamure

#endif // PRE_SURFEL_H_

