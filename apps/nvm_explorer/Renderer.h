#ifndef LAMURE_RENDERER_H
#define LAMURE_RENDERER_H

// #include <lamure/utils.h>
// #include <lamure/types.h>
#include "Camera_View.h"
#include "Scene.h"

#include <iostream>

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
#include <lamure/ren/ray.h>

class Renderer
{
  public:
    // Renderer();
    void init(char **argv, scm::shared_ptr<scm::gl::render_device> device, int width_window, int height_window, std::string name_file_bvh, lamure::ren::Data_Provenance data_provenance);
    bool render(Scene &scene);

    void update_state_lense();
    void translate_sphere(scm::math::vec3f offset);
    void update_radius_sphere(float offset);

    void update_size_point(float scale);

    Camera_View &get_camera_view();

    void toggle_camera(Scene scene);
    void toggle_is_camera_active();
    void previous_camera(Scene scene);
    void next_camera(Scene scene);
    void handle_mouse_movement(int x, int y);

    void start_brushing(int x, int y);

    bool dense_points_only = false;
    bool mode_draw_points_dense = false;
    bool mode_draw_images = true;
    bool mode_draw_lines = false;
    bool mode_draw_cameras = true;
    int mode_prov_data = 0;
    bool dispatch = true;

    bool is_default_camera = true;

    bool is_camera_active = false;
    unsigned long index_current_image_camera = 0;

  private:
    scm::shared_ptr<scm::gl::render_context> _context;
    scm::shared_ptr<scm::gl::render_device> _device;
    scm::gl::program_ptr _program_points;
    scm::gl::program_ptr _program_points_dense;
    scm::gl::program_ptr _program_cameras;
    scm::gl::program_ptr _program_images;
    scm::gl::program_ptr _program_frustra;
    scm::gl::program_ptr _program_lines;
    scm::gl::program_ptr _program_legend;

    lamure::ren::Data_Provenance _data_provenance;

    // scm::shared_ptr<scm::gl::quad_geometry> _quad_legend;
    scm::gl::buffer_ptr _vertex_buffer_object_lines;
    scm::gl::vertex_array_ptr _vertex_array_object_lines;

    scm::gl::rasterizer_state_ptr _rasterizer_state;

    float _size_point_dense = 0.1f;
    float _size_point_sparse_float = 1.0f;
    int _size_point_sparse = 1;

    Camera_View _camera_view;
    lamure::ren::camera *_camera = new lamure::ren::camera();

    float _radius_sphere = 300.0;
    scm::math::vec3f _position_sphere = scm::math::vec3f(0.0f, 0.0f, 0.0f);
    int _state_lense = 0;

    void draw_points_sparse(Scene &scene);
    void draw_cameras(Scene &scene);
    void draw_lines_test();
    void draw_lines(Scene &scene);
    void draw_images(Scene &scene);
    void draw_frustra(Scene &scene);
    bool draw_points_dense(Scene &scene);

    scm::math::vec3f convert_to_world_space(int x, int y, int z);
    // void draw_legend();
};

#endif // LAMURE_RENDERER_H
