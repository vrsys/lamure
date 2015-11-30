#include <lamure/sphere.h>

#include <lamure/bounding_box.h>

namespace lamure
{

const bounding_box sphere::
get_bounding_box() const
{
    return bounding_box(
        vec3r(center_.x - radius_, center_.y - radius_, center_.z - radius_),
        vec3r(center_.x + radius_, center_.y + radius_, center_.z + radius_)
    );
}

bool sphere::
contains(const vec3r& point) const
{
    const real distance_to_center = scm::math::length_sqr(point - center_);
    return distance_to_center <= sqrt(radius_);
}

bool sphere::
is_inside(const bounding_box& bounding_box) const
{
    const vec3r min = bounding_box.min();
    const vec3r max = bounding_box.max();

    real dist_sqr = radius_ * radius_;

    if (center_.x < min.x)
        dist_sqr -= pow((center_.x - min.x), 2);
    else if (center_.x > max.x)
        dist_sqr -= pow((center_.x - max.x), 2);

    if (center_.y < min.y)
        dist_sqr -= pow((center_.y - min.y), 2);
    else if (center_.y > max.y)
        dist_sqr -= pow((center_.y - max.y), 2);

    if (center_.z < min.z)
        dist_sqr -= pow((center_.z - min.z), 2);
    else if (center_.z > max.z)
        dist_sqr -= pow((center_.z - max.z), 2);

    return dist_sqr > 0;

    /*
    const vec3r min = bounding_box.min();
    const vec3r max = bounding_box.max();

    std::vector<vec3r> corners;
    corners.push_back(min);
    corners.push_back(vec3r(min.x, min.y, max.z));
    corners.push_back(vec3r(min.x, max.y, max.z));
    corners.push_back(max);
    corners.push_back(vec3r(max.x, max.y, min.z));
    corners.push_back(vec3r(max.x, min.y, min.z));

    for (auto corner : corners)
    {
        if (contains(corner))
            return true;
    }

    return false;
    */
}

} // namespace lamure
