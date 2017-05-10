#ifndef LAMURE_SCENE_H
#define LAMURE_SCENE_H

#include <scm/core.h>
#include <scm/log.h>
#include <scm/core/pointer_types.h>
#include <scm/core/io/tools.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/viewer/camera.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/primitives/box.h>

#include <scm/core/math.h>

#include <scm/gl_core/gl_core_fwd.h>
#include <scm/gl_util/primitives/primitives_fwd.h>
#include <scm/gl_util/primitives/geometry.h>

#include <scm/gl_util/font/font_face.h>
#include <scm/gl_util/font/text.h>
#include <scm/gl_util/font/text_renderer.h>

#include <scm/core/platform/platform.h>
#include <scm/core/utilities/platform_warning_disable.h>
#include <scm/gl_util/primitives/geometry.h>

#include <lamure/ren/controller.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/policy.h>

#include <boost/assign/list_of.hpp>
#include <memory>

#include <fstream>
#include <vector>
#include <map>
#include <lamure/types.h>
#include <lamure/utils.h>

#include <lamure/ren/cut.h>
#include <lamure/ren/cut_update_pool.h>
#include <vector>

#include "Camera.h"
#include "Point.h"
#include "Struct_Point.h"
#include "Struct_Camera.h"
#include "Struct_Image.h"
#include "Camera_View.h"

class Scene
{

 private:
  std::vector<Camera> _vector_camera;
  std::vector<Point> _vector_point;
  std::vector<Image> _vector_image;

  scm::gl::vertex_array_ptr _vertex_array_object_points;
  scm::gl::vertex_array_ptr _vertex_array_object_cameras;
  scm::gl::vertex_array_ptr _vertex_array_object_images;

  Camera_View _camera_view;

  std::vector<Struct_Point> convert_points_to_struct_point();
  std::vector<Struct_Camera> convert_cameras_to_struct_camera();
  std::vector<Struct_Image> convert_images_to_struct_image();
 public:
  Scene();
  Scene(std::vector<Camera> vector_camera, std::vector<Point> vector_point, std::vector<Image> vector_image);

  void init(scm::shared_ptr<scm::gl::render_device> device);
  scm::gl::vertex_array_ptr get_vertex_array_object_points();
  scm::gl::vertex_array_ptr get_vertex_array_object_cameras();
  scm::gl::vertex_array_ptr get_vertex_array_object_images();

  Camera_View &get_camera_view();
  int count_points();
  int count_cameras();
  int count_images();

  // camera::projection_perspective(float fovy, float aspect, float near_z, float far_z)

};

#endif //LAMURE_SCENE_H
