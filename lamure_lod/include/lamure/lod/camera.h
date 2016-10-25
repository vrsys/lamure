// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_LOD_CAMERA_H_
#define LAMURE_LOD_CAMERA_H_

#include <lamure/platform_lod.h>
#include <lamure/types.h>
#include <lamure/util/trackball.h>
#include <lamure/util/frustum.h>
#include <lamure/math/gl_math.h>

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <mutex>

namespace lamure {
namespace lod {

class LOD_DLL camera
{
public:

  enum class control_type {
    mouse
  };

    struct mouse_state
    {
        bool lb_down_;
        bool mb_down_;
        bool rb_down_;

        mouse_state() : lb_down_(false), mb_down_(false), rb_down_(false)
        {}
    };

                                camera() {};
                                camera(const view_t view_id,
                                    float64_t near_plane,
                                    mat4r_t const& view,
                                    mat4r_t const& proj);

                                camera(const view_t view_id,
                                       mat4r_t const& init_tb_mat,
                                       float64_t distance,
                                       bool fast_travel = false,
                                       bool touch_screen_mode = false);
    virtual                     ~camera();

    void                        event_callback(uint16_t code, float64_t value);

    const view_t                view_id() const { return view_id_; };

    void                        set_projection_matrix(float64_t opening_angle,
                                                    float64_t aspect_ratio,
                                                    float64_t near,
                                                    float64_t far);

    void                        set_view_matrix(mat4r_t const& in_view );

    void                        calc_view_to_screen_space_matrix(vec2f_t const& win_dimensions);
    void                        set_trackball_matrix(mat4r_t const& tb_matrix) { trackball_.set_transform(tb_matrix); }
    const mat4r_t&     trackball_matrix() const { return trackball_.transform(); };

    lamure::util::frustum_t::classification_result_t const cull_against_frustum(lamure::util::frustum_t const& frustum, lamure::math::bounding_box_t const& b) const;

    lamure::util::frustum_t const get_frustum_by_model(mat4r_t const& model) const;

    void                        update_trackball_mouse_pos(float64_t x, float64_t y);
    void                        update_trackball(int32_t x, int32_t y,
                                                int32_t window_width, int32_t window_height,
                                                mouse_state const& mouse_state);

    void                        set_dolly_sens_(float64_t ds) { dolly_sens_ = ds; }

    lamure::util::frustum_t const      get_predicted_frustum(mat4r_t const& in_cam_or_mat);

    inline const float64_t          near_plane_value() const { return near_plane_value_; }
    inline const float64_t          far_plane_value() const {return far_plane_value_; }


    std::vector<vec3r_t>     get_frustum_corners() const;

    mat4r_t const get_view_matrix() const;
    mat4r_t const get_high_precision_view_matrix() const;
    mat4r_t const get_projection_matrix() const;

    void                        write_view_matrix(std::ofstream& matrix_stream);
    void                        set_trackball_center_of_rotation(const vec3r_t& cor);
protected:

    enum camera_state
    {
        CAM_STATE_GUA,
        CAM_STATE_LAMURE,
        INVALID_CAM_STATE
    };


    float64_t const                 remap_value(float64_t value, float64_t oldMin, float64_t oldMax,
                                           float64_t newMin, float64_t newMax) const;
    float64_t const                 transfer_values(float64_t currentValue, float64_t maxValue) const;


private:

    view_t                      view_id_;

    mat4r_t                     view_matrix_;
    mat4r_t                     projection_matrix_;
    lamure::util::frustum_t     frustum_;

    float64_t                       near_plane_value_;
    float64_t                       far_plane_value_;

    lamure::util::trackball_t   trackball_;

    float64_t                      trackball_init_x_;
    float64_t                      trackball_init_y_;

    float64_t                      dolly_sens_;

    control_type                controlType_;

    static std::mutex           transform_update_mutex_;

    bool                        is_in_touch_screen_mode_;

    float64_t                      sum_trans_x_;
    float64_t                      sum_trans_y_;
    float64_t                      sum_trans_z_;
    float64_t                      sum_rot_x_;
    float64_t                      sum_rot_y_;
    float64_t                      sum_rot_z_;

    camera_state                    cam_state_;
};

} } // namespace lamure

#endif // LAMURE_LOD_CAMERA_H_

