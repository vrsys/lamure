#ifndef LAMURE_DENSE_POINT_H
#define LAMURE_DENSE_POINT_H

#include "Point.h"

namespace prov
{
class DensePoint : public Point
{
  public:
    DensePoint(long _index, const vec3d &_center, const vec3d &_color, const vec<char> &_metadata, const vec3d &_normal);
    ~DensePoint();
    const vec3d &get_normal() const;

  protected:
    vec3d _normal;
};
}

#endif // LAMURE_DENSE_POINT_H