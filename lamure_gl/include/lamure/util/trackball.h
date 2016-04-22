
#ifndef LAMURE_UTIL_TRACKBALL_H_
#define LAMURE_UTIL_TRACKBALL_H_

#include <lamure/math.h>
#include <lamure/types.h>
#include <lamure/platform_gl.h>
#include <lamure/math/gl_math.h>

namespace lamure {
namespace util {

class trackball_t
{
public:
  trackball_t();
  ~trackball_t();

  void rotate(float64_t fx, float64_t fy, float64_t tx, float64_t ty);
  void translate(float64_t x, float64_t y);
  void dolly(float64_t y);

  const lamure::math::mat4d_t& transform() const { return transform_; };
  void set_transform(const lamure::math::mat4d_t& transform) { transform_ = transform; };
  float64_t dolly() const { return dolly_; };
  void set_dolly(const float64_t dolly) { dolly_ = dolly; };

private:
  float64_t project_to_sphere(float64_t x, float64_t y) const;

  lamure::math::mat4d_t transform_;
  float64_t radius_;
  float64_t dolly_;

};

} // namespace util
} // namespace lamure

#endif
