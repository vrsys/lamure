#ifndef LAMURE_IMAGE_H
#define LAMURE_IMAGE_H

#include <string>
#include "exif.h"
#include <iostream>
#include <scm/core/math.h>
#include <lamure/ren/camera.h>
#include <lamure/ren/cut.h>

#include <lamure/ren/controller.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/controller.h>
#include <lamure/ren/policy.h>

#include <boost/assign/list_of.hpp>
#include <memory>

#include <fstream>

#include <scm/core.h>
#include <scm/log.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>
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

using namespace std;

class Image
{
 private:

  int _height;
  int _width;
  string _file_name;
  double _focal_length;
  double _fp_resolution_x;
  double _fp_resolution_y;
  scm::gl::texture_2d_ptr _texture;
  scm::gl::sampler_state_ptr _state;

  scm::math::mat4f _transformation = scm::math::mat4f::identity();

 public:

  Image ();

  Image (int _height,
         int _width,
         const string &_file_name,
         double _focal_length,
         double _fp_resolution_x,
         double _fp_resolution_y);

  int get_height () const;

  void set_height (int _height);

  int get_width () const;

  void set_width (int _width);

  const string &get_file_name () const;

  void set_file_name (const string &_file_name);

  double get_focal_length () const;

  void set_focal_length (double _focal_length);

  double get_fp_resolution_x () const;

  void set_fp_resolution_x (double _fp_resolution_x);

  double get_fp_resolution_y () const;

  void set_fp_resolution_y (double _fp_resolution_y);

  scm::gl::sampler_state_ptr get_state();

  float get_width_world();
  float get_height_world();

  const scm::math::mat4f& get_transformation() const;

  void update_transformation(scm::math::mat4f transformation, float scale);

  static Image read_from_file(const string &_file_name);

  void load_texture(scm::shared_ptr<scm::gl::render_device> device);
  scm::gl::texture_2d_ptr get_texture();
};

#endif //LAMURE_IMAGE_H
