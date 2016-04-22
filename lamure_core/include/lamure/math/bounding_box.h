// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_MATH_BOUNDING_BOX_H_
#define LAMURE_MATH_BOUNDING_BOX_H_

#include <lamure/platform_core.h>
#include <lamure/types.h>
#include <lamure/math/bounding_sphere.h>
#include <lamure/assert.h>

namespace lamure {
namespace math {

class CORE_DLL bounding_box_t
{
public:

    explicit            bounding_box_t() : min_(vec3r_t(1.0)), max_(vec3r_t(-1.0)) {}

    explicit            bounding_box_t(const vec3r_t& min,
                                    const vec3r_t& max)
                            : min_(min), max_(max) {

                                ASSERT((min_[0] <= max_[0]) && 
                                       (min_[1] <= max_[1]) && 
                                       (min_[2] <= max_[2]));
                            }

    const vec3r_t         min() const { return min_; }
    vec3r_t&              min() { return min_; }

    const vec3r_t         max() const { return max_; }
    vec3r_t&              max() { return max_; }

    const bool          is_invalid() const { 
                            return min_.x_ > max_.x_ || 
                                   min_.y_ > max_.y_ || 
                                   min_.z_ > max_.z_; 
                        }

    const bool          is_valid() const { return !is_invalid(); }

    const vec3r_t         get_dimensions() const { 
                            ASSERT(is_valid());
                            return max_ - min_;
                        }

    const vec3r_t         get_center() const { 
                            ASSERT(is_valid());
                            return (max_ + min_) / 2.0;
                        }

    const uint8_t       get_longest_axis() const;
    const uint8_t       get_shortest_axis() const;

    const bool          contains(const vec3r_t& point) const {
                            ASSERT(is_valid());
                            return min_.x_ <= point.x_ && point.x_ <= max_.x_ &&
                                   min_.y_ <= point.y_ && point.y_ <= max_.y_ &&
                                   min_.z_ <= point.z_ && point.z_ <= max_.z_;
                        }

    const bool          contains(const bounding_box_t& bounding_box) const {
                            ASSERT(is_valid());
                            ASSERT(bounding_box.is_valid());
                            return contains(bounding_box.min()) &&
                                   contains(bounding_box.max());
                        }

    const bool          contains(const bounding_sphere_t& sphere) const {
                            ASSERT(is_valid());
                            return contains(sphere.get_bounding_box());
                        }

    const bool          intersects(const bounding_box_t& bounding_box) const {
                            ASSERT(is_valid());
                            ASSERT(bounding_box.is_valid());
                            return !(max_.x_ < bounding_box.min().x_ || 
                                     max_.y_ < bounding_box.min().y_ || 
                                     max_.z_ < bounding_box.min().z_ || 
                                     min_.x_ > bounding_box.max().x_ || 
                                     min_.y_ > bounding_box.max().y_ ||
                                     min_.z_ > bounding_box.max().z_);
                        }

    void                expand(const vec3r_t& point);

    void                expand(const vec3r_t& point, const real_t radius);

    void                expand(const bounding_box_t& bounding_box);

    void                expand_by_disk(const vec3r_t& surfel_center, const vec3f_t& surfel_normal, const real_t surfel_radius);

    void                shrink(const bounding_box_t& bounding_box);

    inline bool         operator==(const bounding_box_t& rhs) const
                            { return min_ == rhs.min_ && max_ == rhs.max_; }
    inline bool         operator!=(const bounding_box_t& rhs) const
                            { return !(operator==(rhs)); }

private:

    vec3r_t               min_;
    vec3r_t               max_;

};

} // namespace math
} // namespace lamure

#endif // LAMURE_MATH_BOUNDING_BOX_H_

