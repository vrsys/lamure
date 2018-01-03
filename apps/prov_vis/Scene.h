#ifndef LAMURE_SCENE_H
#define LAMURE_SCENE_H

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

#include <lamure/prov/sparse_cache.h>
#include <lamure/prov/camera.h>
#include <lamure/prov/sparse_point.h>

#include "Camera_Custom.h"
#include "Struct_Camera.h"
#include "Struct_Line.h"
#include "Struct_Point.h"
#include <scm/gl_util/primitives/quad.h>

class Scene
{
  public:
    Scene();
    Scene(std::vector<lamure::prov::SparsePoint> &vector_point, std::vector<lamure::prov::Camera> &vector_camera);

    void init(scm::shared_ptr<scm::gl::render_device> device, std::string const& image_directory);
    scm::gl::vertex_array_ptr get_vertex_array_object_points();
    scm::gl::vertex_array_ptr get_vertex_array_object_cameras();
    scm::shared_ptr<scm::gl::quad_geometry> get_quad();

    // std::vector<Image> &get_vector_image();
    std::vector<Camera_Custom> &get_vector_camera();
    std::vector<lamure::prov::SparsePoint> &get_vector_point();
    bool update(float time_delta);
    void update_scale_frustum(float offset);

    int count_points();
    int count_cameras();
    // int count_images();

    void toggle_camera();
    void previous_camera();
    void next_camera();

  private:
    std::vector<Camera_Custom> _vector_camera;
    std::vector<lamure::prov::SparsePoint> _vector_point;
    // std::vector<Image> _vector_image;

    scm::gl::vertex_array_ptr _vertex_array_object_points;
    scm::gl::buffer_ptr _vertex_buffer_object_points;
    scm::gl::vertex_array_ptr _vertex_array_object_cameras;
    scm::gl::buffer_ptr _vertex_buffer_object_cameras;

    scm::shared_ptr<scm::gl::quad_geometry> _quad;

    std::vector<Struct_Point> convert_points_to_struct_point();
    std::vector<Struct_Camera> convert_cameras_to_struct_camera();

    bool is_default_camera = true;
    int index_current_image_camera = 0;
};

#endif // LAMURE_SCENE_H
