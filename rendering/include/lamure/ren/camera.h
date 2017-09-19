// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_CAMERA_H_
#define REN_CAMERA_H_

#include <scm/gl_core.h>
#include <scm/gl_core/primitives/frustum.h>
#include <scm/gl_util/viewer/camera.h>

#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include <lamure/ren/platform.h>
#include <lamure/ren/trackball.h>
#include <lamure/types.h>

namespace lamure
{
namespace ren
{
class RENDERING_DLL camera
{
  public:
    enum class control_type
    {
        mouse
    };

    struct mouse_state
    {
        bool lb_down_;
        bool mb_down_;
        bool rb_down_;

        mouse_state() : lb_down_(false), mb_down_(false), rb_down_(false) {}
    };

    camera() : cam_state_(CAM_STATE_GUA){};
    camera(const view_t view_id, float near_plane, scm::math::mat4f const &view, scm::math::mat4f const &proj);

    camera(const view_t view_id, scm::math::mat4f const &init_tb_mat, float distance, bool fast_travel = false, bool touch_screen_mode = false);
    virtual ~camera();

    void event_callback(uint16_t code, float value);

    const view_t view_id() const { return view_id_; };

    void set_projection_matrix(float opening_angle, float aspect_ratio, float near, float far);

    void set_view_matrix(scm::math::mat4d const &in_view);

    void calc_view_to_screen_space_matrix(scm::math::vec2f const &win_dimensions);
    void set_trackball_matrix(scm::math::mat4d const &tb_matrix) { trackball_.set_transform(tb_matrix); }
    const scm::math::mat4d &trackball_matrix() const { return trackball_.transform(); };

    scm::gl::frustum::classification_result const cull_against_frustum(scm::gl::frustum const &frustum, scm::gl::box const &b) const;

    scm::gl::frustum const get_frustum_by_model(scm::math::mat4 const &model) const;

    void update_trackball_mouse_pos(double x, double y);
    void update_trackball(int x, int y, int window_width, int window_height, mouse_state const &mouse_state);

    void set_dolly_sens_(double ds) { dolly_sens_ = ds; }

    scm::gl::frustum const get_predicted_frustum(scm::math::mat4f const &in_cam_or_mat);

    inline const float near_plane_value() const { return near_plane_value_; }
    inline const float far_plane_value() const { return far_plane_value_; }

    std::vector<scm::math::vec3d> get_frustum_corners() const;

    scm::math::mat4f const get_view_matrix() const;
    scm::math::mat4d const get_high_precision_view_matrix() const;
    scm::math::mat4f const get_projection_matrix() const;

    void write_view_matrix(std::ofstream &matrix_stream);
    void set_trackball_center_of_rotation(const scm::math::vec3f &cor);

  protected:
    enum camera_state
    {
        CAM_STATE_GUA,
        CAM_STATE_LAMURE,
        INVALID_CAM_STATE
    };

    float const remap_value(float value, float oldMin, float oldMax, float newMin, float newMax) const;
    float const transfer_values(float currentValue, float maxValue) const;

  private:
    view_t view_id_;

    scm::math::mat4f view_matrix_;
    scm::math::mat4f projection_matrix_;
    scm::gl::frustum frustum_;

    float near_plane_value_;
    float far_plane_value_;

    lamure::ren::trackball trackball_;

    double trackball_init_x_;
    double trackball_init_y_;

    double dolly_sens_;

    control_type controlType_;

    static std::mutex transform_update_mutex_;

    bool is_in_touch_screen_mode_;

    double sum_trans_x_;
    double sum_trans_y_;
    double sum_trans_z_;
    double sum_rot_x_;
    double sum_rot_y_;
    double sum_rot_z_;

    camera_state cam_state_;
};
}
} // namespace lamure

#endif // REN_CAMERA_H_
