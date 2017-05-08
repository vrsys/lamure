#ifndef LAMURE_CAMERA_H
#define LAMURE_CAMERA_H

#include <scm/core/math/quat.h>
#include <scm/core/math/vec3.h>
#include <scm/core/math.h>
#include "Image.h"

using namespace scm::math;

class Camera
{
 private:

  Image _still_image;
  double _focal_length;
  quat<double> _orientation;
  vec3d _center;
  double _radial_distortion;

 public:
  const Image &get_still_image () const;

  void set_still_image (const Image &_still_image);

  double get_focal_length () const;

  void set_focal_length (double _focal_length);

  const quat<double> &get_orientation () const;

  void set_orientation (const quat<double> &_orientation);

  const vec3d &get_center () const;

  void set_center (const vec<double, 3> &_center);

  double get_radial_distortion () const;

  void set_radial_distortion (double _radial_distortion);

  Camera ();

  Camera (const Image &_still_image, double _focal_length, const quat<double> &_orientation,
          const vec<double, 3> &_center, double _radial_distortion);

};

#endif //LAMURE_CAMERA_H
