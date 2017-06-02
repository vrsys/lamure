#ifndef LAMURE_LOD_POINT_H
#define LAMURE_LOD_POINT_H

#include "DensePoint.h"

namespace prov
{
class LoDPoint : public DensePoint
{
  public:
    LoDPoint(long _index, const vec3d &_center, const vec3d &_color, const vec<char> &_metadata, const vec3d &_normal, double _deviation_of_normals);
    ~LoDPoint();
    double get_deviation_of_normals() const;

  protected:
    double _deviation_of_normals;
};
}

#endif // LAMURE_LOD_POINT_H