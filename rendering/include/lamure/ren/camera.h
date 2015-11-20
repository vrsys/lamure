// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_CAMERA_H_
#define REN_CAMERA_H_

#include <scm/gl_util/viewer/camera.h>
#include <scm/gl_core.h>
#include <scm/gl_core/primitives/frustum.h>

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <mutex>

#include <lamure/ren/platform.h>
#include <lamure/types.h>
#include <lamure/ren/trackball.h>


namespace lamure {
namespace ren {

class RENDERING_DLL Camera
{
public:

  enum class control_type {
    mouse
  };

    struct MouseState
    {
        bool lb_down_;
        bool mb_down_;
        bool rb_down_;

        MouseState() : lb_down_(false), mb_down_(false), rb_down_(false)
        {}
    };

                                Camera() {};
                                Camera(const view_t view_id,
                                    float near_plane,
                                    scm::math::mat4f const& view,
                                    scm::math::mat4f const& proj);

                                Camera(const view_t view_id,
                                       scm::math::mat4f const& init_tb_mat,
                                       float distance,
                                       bool fast_travel = false,
                                       bool touch_screen_mode = false);
    virtual                     ~Camera();

    void                        event_callback(uint16_t code, float value);

    const view_t                view_id() const { return view_id_; };

    void                        SetProjectionMatrix(float opening_angle,
                                                    float aspect_ratio,
                                                    float near,
                                                    float far);

    void                        CalcViewToScreenSpaceMatrix(scm::math::vec2f const& win_dimensions);
    void                        SetTrackballMatrix(scm::math::mat4d const& tb_matrix) { trackball_.set_transform(tb_matrix); }
    const scm::math::mat4d&     trackball_matrix() const { return trackball_.transform(); };

    scm::gl::frustum::classification_result const CullAgainstFrustum(scm::gl::frustum const& frustum,
                                                               scm::gl::box const & b) const;

    scm::gl::frustum const GetFrustumByModel(scm::math::mat4 const& model) const;

    void                        UpdateTrackballMousePos(double x, double y);
    void                        UpdateTrackball(int x, int y,
                                                int window_width, int window_height,
                                                MouseState const& mouse_state);

    void                        set_dolly_sens_(double ds) { dolly_sens_ = ds; }

    scm::gl::frustum const      GetPredictedFrustum(scm::math::mat4f const& in_cam_or_mat);

    inline const float          near_plane_value() const { return near_plane_value_; }
    inline const float          far_plane_value() const {return far_plane_value_; }


    std::vector<scm::math::vec3d>     get_frustum_corners() const;

    scm::math::mat4f const GetViewMatrix() const;
    scm::math::mat4d const GetHighPrecisionViewMatrix() const;
    scm::math::mat4f const GetProjectionMatrix() const;

    void                        WriteViewMatrix(std::ofstream& matrix_stream);
    void                        SetTrackballCenterOfRotation(const scm::math::vec3f& cor);
protected:

    enum CamState
    {
        CAM_STATE_GUA,
        CAM_STATE_LAMURE,
        INVALID_CAM_STATE
    };


    float const                 RemapValue(float value, float oldMin, float oldMax,
                                           float newMin, float newMax) const;
    float const                 TransferValues(float currentValue, float maxValue) const;


private:

    view_t                      view_id_;

    scm::math::mat4f            view_matrix_;
    scm::math::mat4f            projection_matrix_;
    scm::gl::frustum            frustum_;

    float                       near_plane_value_;
    float                       far_plane_value_;

    lamure::ren::Trackball         trackball_;

    double                      trackball_init_x_;
    double                      trackball_init_y_;

    double                      dolly_sens_;

    control_type                controlType_;

    static std::mutex           transform_update_mutex_;

    bool                        is_in_touch_screen_mode_;

    double                      sum_trans_x_;
    double                      sum_trans_y_;
    double                      sum_trans_z_;
    double                      sum_rot_x_;
    double                      sum_rot_y_;
    double                      sum_rot_z_;

    CamState                    cam_state_;
};

} } // namespace lamure

#endif // REN_CAMERA_H_

