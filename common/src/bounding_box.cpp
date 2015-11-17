#include <lamure/bounding_box.h>

#include <algorithm>

namespace lamure
{

const uint8_t BoundingBox::
GetLongestAxis() const
{
    const vec3r d = GetDimensions();
    return d.x > d.y ?
        (d.x > d.z ? 0 : (d.y > d.z ? 1 : 2)) :
        (d.y > d.z ? 1 : 2);
}

const uint8_t BoundingBox::
GetShortestAxis() const
{
    const vec3r d = GetDimensions();
    return d.x < d.y ?
        (d.x < d.z ? 0 : (d.y < d.z ? 1 : 2)) :
        (d.y < d.z ? 1 : 2);
}

void BoundingBox::
Expand(const vec3r& point)
{
    if (IsValid()) {
        min_.x = std::min(min_.x, point.x);
        min_.y = std::min(min_.y, point.y);
        min_.z = std::min(min_.z, point.z);

        max_.x = std::max(max_.x, point.x);
        max_.y = std::max(max_.y, point.y);
        max_.z = std::max(max_.z, point.z);
    }
    else {
        min_ = max_ = point;
    }
}

void BoundingBox::
Expand(const vec3r& point, const real radius)
{
    if (IsValid()) {
      min_.x = std::min(min_.x, point.x - radius);
      min_.y = std::min(min_.y, point.y - radius);
      min_.z = std::min(min_.z, point.z - radius);
                                
      max_.x = std::max(max_.x, point.x + radius);
      max_.y = std::max(max_.y, point.y + radius);
      max_.z = std::max(max_.z, point.z + radius);
    }
    else {
        min_ = point - radius;
        max_ = point + radius;
    }
}

void BoundingBox::
Expand(const BoundingBox& bounding_box)
{
    if (bounding_box.IsValid()) {
        if (IsValid()) {
            min_.x = std::min(min_.x, bounding_box.min().x);
            min_.y = std::min(min_.y, bounding_box.min().y);
            min_.z = std::min(min_.z, bounding_box.min().z);

            max_.x = std::max(max_.x, bounding_box.max().x);
            max_.y = std::max(max_.y, bounding_box.max().y);
            max_.z = std::max(max_.z, bounding_box.max().z);
        }
        else {
            *this = bounding_box;
        }
    }
}

void BoundingBox::
Shrink(const BoundingBox& bounding_box)
{
    assert(Contains(bounding_box));


}

} // namespace lamure
