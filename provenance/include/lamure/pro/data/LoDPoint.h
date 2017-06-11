#ifndef LAMURE_LOD_POINT_H
#define LAMURE_LOD_POINT_H

#include "lamure/pro/common.h"
#include "DensePoint.h"

namespace prov
{
class LoDPoint : public DensePoint
{
  public:
    LoDPoint::LoDPoint(long _index, const vec3d &_center, const vec3d &_color, const vec<char> &_metadata, const vec3d &_normal, double _deviation_of_normals)
        : DensePoint(_index, _center, _color, _metadata, _normal), _deviation_of_normals(_deviation_of_normals)
    {
    }
    LoDPoint::~LoDPoint() {}
    double LoDPoint::get_deviation_of_normals() const { return _deviation_of_normals; }
  protected:
    double _deviation_of_normals;
};
}

#endif // LAMURE_LOD_POINT_H