#ifndef LAMURE_POINT_H
#define LAMURE_POINT_H

#include "MetaContainer.h"
#include "lamure/pro/common.h"

namespace prov
{
class Point : public MetaContainer
{
  public:
    Point(long _index, const vec3d &_center, const vec3d &_color, const vec<char> &_metadata);
    ~Point();
    long get_index() const;
    const vec3d &get_center() const;
    const vec3d &get_color() const;

  protected:
    long _index;
    vec3d _center;
    vec3d _color;
};
}

#endif // LAMURE_POINT_H