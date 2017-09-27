#ifndef LAMURE_FRUSTUM_H
#define LAMURE_FRUSTUM_H

#include <scm/core.h>
#include <scm/core/io/tools.h>
#include <scm/core/pointer_types.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>
#include <scm/log.h>

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/primitives/box.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/viewer/camera.h>

#include <scm/core/math.h>

#include <scm/gl_core/gl_core_fwd.h>
#include <scm/gl_util/primitives/geometry.h>
#include <scm/gl_util/primitives/primitives_fwd.h>

#include <scm/gl_util/font/font_face.h>
#include <scm/gl_util/font/text.h>
#include <scm/gl_util/font/text_renderer.h>

#include <scm/core/platform/platform.h>
#include <scm/core/utilities/platform_warning_disable.h>
#include <scm/gl_util/primitives/geometry.h>

#include <lamure/ren/controller.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/policy.h>

#include <boost/assign/list_of.hpp>
#include <memory>

#include <fstream>
#include <lamure/types.h>
#include <lamure/utils.h>
#include <map>
#include <vector>

#include <lamure/ren/cut.h>
#include <lamure/ren/cut_update_pool.h>
#include <vector>

#include "Struct_Line.h"

class Frustum
{
  public:
    Frustum();
    Frustum(float image_width_world, float image_height_world, double focal_length);

    void init(scm::shared_ptr<scm::gl::render_device> device);
    void update_scale_frustum(float offset);
    float &get_scale();
    scm::gl::vertex_array_ptr const &get_vertex_array_object();

  private:
    scm::math::vec3f _near_left_top = scm::math::vec3f(-0.5, 0.5, 0.0);
    scm::math::vec3f _near_right_top = scm::math::vec3f(0.5, 0.5, 0.0);
    scm::math::vec3f _near_left_bottom = scm::math::vec3f(-0.5, -0.5, 0.0);
    scm::math::vec3f _near_right_bottom = scm::math::vec3f(0.5, -0.5, 0.0);
    scm::math::vec3f _far_left_top = scm::math::vec3f(-1.0, 1.0, 0.0);
    scm::math::vec3f _far_right_top = scm::math::vec3f(1.0, 1.0, 0.0);
    scm::math::vec3f _far_left_bottom = scm::math::vec3f(-1.0, -1.0, 0.0);
    scm::math::vec3f _far_right_bottom = scm::math::vec3f(1.0, -1.0, 0.0);

    float _image_width_world;
    float _image_height_world;
    double _focal_length;

    float _scale = 200.0f;

    scm::gl::vertex_array_ptr _vertex_array_object;
    scm::gl::buffer_ptr _vertex_buffer_object;

    std::vector<Struct_Line> convert_frustum_to_struct_line();
    // void Frustum::update_frustum_points();
    void calc_frustum_points();
};

#endif // LAMURE_FRUSTUM_H
