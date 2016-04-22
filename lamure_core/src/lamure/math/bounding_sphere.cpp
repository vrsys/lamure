
#include <lamure/math/bounding_sphere.h>
#include <lamure/math/bounding_box.h>

namespace lamure {
namespace math {

const bounding_box_t bounding_sphere_t::
get_bounding_box() const
{
    return bounding_box_t(
        vec3r_t(center_.x_ - radius_, center_.y_ - radius_, center_.z_ - radius_),
        vec3r_t(center_.x_ + radius_, center_.y_ + radius_, center_.z_ + radius_)
    );
}

bool bounding_sphere_t::
contains(const vec3r_t& point) const
{
    const real_t distance_to_center = lamure::math::length_sqr(point - center_);
    return distance_to_center <= sqrt(radius_);
}

real_t bounding_sphere_t::
clamp_to_AABB_face(real_t actual_value, real_t min_BB_value, real_t max_BB_value) const {
    return std::max(std::min(actual_value, max_BB_value), min_BB_value);
}

vec3r_t bounding_sphere_t::
get_closest_point_on_AABB(const bounding_box_t& bounding_box) const {
    const vec3r_t min = bounding_box.min();
    const vec3r_t max = bounding_box.max();

    vec3r_t closest_point_on_AABB(0.0, 0.0, 0.0);

    for(int dim_idx = 0; dim_idx < 3; ++dim_idx) {
        closest_point_on_AABB[dim_idx] 
            = clamp_to_AABB_face(center_[dim_idx], min[dim_idx], max[dim_idx]);
    }

    return closest_point_on_AABB;
}

bool bounding_sphere_t::
intersects_or_contains(const bounding_box_t& bounding_box) const
{
    vec3r_t closest_point_on_AABB = get_closest_point_on_AABB(bounding_box);

    return ( lamure::math::length_sqr( center_ - closest_point_on_AABB ) <= radius_*radius_);
}

} // namespace math
} // namespace lamure
