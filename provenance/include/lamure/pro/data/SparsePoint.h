#ifndef LAMURE_SPARSE_POINT_H
#define LAMURE_SPARSE_POINT_H

#include "Camera.h"
#include "Point.h"

namespace prov
{
class SparsePoint : public Point
{
  public:
    class Measurement
    {
      public:
        Measurement(const Camera &_camera, const vec2d &_occurence);
        ~Measurement();
        const Camera &get_camera() const;
        const vec2d &get_occurence() const;

      private:
        Camera _camera;
        vec2d _occurence;
    };

    SparsePoint(long _index, const vec3d &_center, const vec3d &_color, const vec<char> &_metadata, const vec<prov::SparsePoint::Measurement> &_measurements);
    ~SparsePoint();
    const vec<prov::SparsePoint::Measurement> &get_measurements() const;

  protected:
    vec<Measurement> _measurements;
};
}

#endif // LAMURE_SPARSE_POINT_H