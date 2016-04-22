
#ifndef LAMURE_UTIL_FRUSTUM_H_
#define LAMURE_UTIL_FRUSTUM_H_

#include <lamure/math.h>
#include <lamure/types.h>
#include <lamure/math/plane.h>
#include <lamure/math/bounding_box.h>
#include <lamure/platform_gl.h>
#include <lamure/assert.h>

namespace lamure {
namespace util {

class frustum_t
{
public:
  frustum_t(const mat4r_t& _mvp = mat4r_t::identity());
  ~frustum_t();

  enum plane_id_t {
    LEFT = 0,
    RIGHT = 1,
    TOP = 2,
    BOTTOM = 3,
    NEAR = 4,
    FAR = 5
  };

  enum classification_result_t {
    INSIDE,
    OUTSIDE,
    INTERSECTING
  };

  const lamure::math::plane_t& get_plane(uint32_t _p) const;
  void transform(const mat4r_t& _t);
  classification_result_t classify(const lamure::math::bounding_box_t& _box) const;
  classification_result_t classify(const lamure::math::bounding_sphere_t& _sphere) const;

private:
  void update(const mat4r_t& _mvp);

  std::vector<lamure::math::plane_t> planes_;

};

} // namespace util
} // namespace lamure

#endif
