#ifndef LAMURE_DENSE_POINT_H
#define LAMURE_DENSE_POINT_H

#include "Point.h"
#include "lamure/pro/common.h"

namespace prov
{
class DensePoint : public Point
{
  public:
    DensePoint() { _normal = vec3d(); }
    DensePoint(long _index, const vec3d &_center, const vec3d &_color, const vec<char> &_metadata, const vec3d &_normal) : Point(_index, _center, _color, _metadata), _normal(_normal) {}
    ~DensePoint() {}
    const vec3d &get_normal() const { return _normal; }
    friend ifstream &operator>>(ifstream &is, DensePoint &dense_point)
    {
        dense_point.read_essentials(is);

        is.read(reinterpret_cast<char *>(&dense_point._normal[0]), 8);
        dense_point._normal[0] = swap(dense_point._normal[0], true);
        is.read(reinterpret_cast<char *>(&dense_point._normal[1]), 8);
        dense_point._normal[1] = swap(dense_point._normal[1], true);
        is.read(reinterpret_cast<char *>(&dense_point._normal[2]), 8);
        dense_point._normal[2] = swap(dense_point._normal[2], true);

        if(DEBUG)
            printf("\nNormal: %f %f %f", dense_point._normal[0], dense_point._normal[1], dense_point._normal[2]);

        return is;
    }

  protected:
    vec3d _normal;
};
}

#endif // LAMURE_DENSE_POINT_H