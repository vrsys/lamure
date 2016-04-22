
#include <lamure/math/bounding_box.h>

#include <algorithm>

namespace lamure {
namespace math {

const uint8_t bounding_box_t::
get_longest_axis() const
{
    const vec3r_t d = get_dimensions();
    return d.x_ > d.y_ ?
        (d.x_ > d.z_ ? 0 : (d.y_ > d.z_ ? 1 : 2)) :
        (d.y_ > d.z_ ? 1 : 2);
}

const uint8_t bounding_box_t::
get_shortest_axis() const
{
    const vec3r_t d = get_dimensions();
    return d.x_ < d.y_ ?
        (d.x_ < d.z_ ? 0 : (d.y_ < d.z_ ? 1 : 2)) :
        (d.y_ < d.z_ ? 1 : 2);
}

void bounding_box_t::
expand(const vec3r_t& point)
{
    if (is_valid()) {
        min_.x_ = std::min(min_.x_, point.x_);
        min_.y_ = std::min(min_.y_, point.y_);
        min_.z_ = std::min(min_.z_, point.z_);

        max_.x_ = std::max(max_.x_, point.x_);
        max_.y_ = std::max(max_.y_, point.y_);
        max_.z_ = std::max(max_.z_, point.z_);
    }
    else {
        min_ = max_ = point;
    }
}

void bounding_box_t::
expand(const vec3r_t& point, const real_t radius)
{
    if (is_valid()) {
      min_.x_ = std::min(min_.x_, point.x_ - radius);
      min_.y_ = std::min(min_.y_, point.y_ - radius);
      min_.z_ = std::min(min_.z_, point.z_ - radius);
                                
      max_.x_ = std::max(max_.x_, point.x_ + radius);
      max_.y_ = std::max(max_.y_, point.y_ + radius);
      max_.z_ = std::max(max_.z_, point.z_ + radius);
    }
    else {
        min_ = point - radius;
        max_ = point + radius;
    }
}

void bounding_box_t::
expand(const bounding_box_t& bounding_box)
{
    if (bounding_box.is_valid()) {
        if (is_valid()) {
            min_.x_ = std::min(min_.x_, bounding_box.min().x_);
            min_.y_ = std::min(min_.y_, bounding_box.min().y_);
            min_.z_ = std::min(min_.z_, bounding_box.min().z_);

            max_.x_ = std::max(max_.x_, bounding_box.max().x_);
            max_.y_ = std::max(max_.y_, bounding_box.max().y_);
            max_.z_ = std::max(max_.z_, bounding_box.max().z_);
        }
        else {
            *this = bounding_box;
        }
    }
}

void bounding_box_t::
expand_by_disk(const vec3r_t& surfel_center, const vec3f_t& surfel_normal, const real_t surfel_radius) {

    const uint8_t X_AXIS = 0;
    const uint8_t Y_AXIS = 1;
    const uint8_t Z_AXIS = 2;

    auto calculate_half_offset = [](const vec3r_t& surf_center,
                                    const vec3r_t surf_normal,
                                    const real_t surf_radius, uint8_t axis) {
        vec3r_t point_along_axis(surf_center);

        point_along_axis[axis] += surf_radius;

        vec3r_t projected_point = point_along_axis - lamure::math::dot(point_along_axis - surf_center, surf_normal) * surf_normal;

        return std::fabs(surf_center[axis] - projected_point[axis]);
    };

    vec3r_t normal = vec3r_t(surfel_normal);

    real_t half_offset_x = calculate_half_offset(surfel_center, normal, surfel_radius, X_AXIS);
    real_t half_offset_y = calculate_half_offset(surfel_center, normal, surfel_radius, Y_AXIS);
    real_t half_offset_z = calculate_half_offset(surfel_center, normal, surfel_radius, Z_AXIS);

    if (is_valid()) {
      min_.x_ = std::min(min_.x_, surfel_center.x_ - half_offset_x);
      min_.y_ = std::min(min_.y_, surfel_center.y_ - half_offset_y);
      min_.z_ = std::min(min_.z_, surfel_center.z_ - half_offset_z);

      max_.x_ = std::max(max_.x_, surfel_center.x_ + half_offset_x);
      max_.y_ = std::max(max_.y_, surfel_center.y_ + half_offset_y);
      max_.z_ = std::max(max_.z_, surfel_center.z_ + half_offset_z);
    }
    else {
      min_.x_ = surfel_center.x_ - half_offset_x;
      min_.y_ = surfel_center.y_ - half_offset_y;
      min_.z_ = surfel_center.z_ - half_offset_z;

      max_.x_ = surfel_center.x_ + half_offset_x;
      max_.y_ = surfel_center.y_ + half_offset_y;
      max_.z_ = surfel_center.z_ + half_offset_z;
    }
}

void bounding_box_t::
shrink(const bounding_box_t& bounding_box)
{
    ASSERT(contains(bounding_box));


}

} // namespace math
} // namespace lamure
