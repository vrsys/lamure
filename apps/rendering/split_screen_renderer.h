// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_APP_SCREEN_RENDERER_H_
#define REN_APP_SCREEN_RENDERER_H_

#include <lamure/ren/camera.h>
#include <lamure/ren/cut.h>

#include <lamure/ren/model_database.h>
#include <lamure/ren/cut_database.h>
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
#include <scm/gl_util/manipulators/trackball_manipulator.h>
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

#include <vector>
#include <map>
#include <lamure/types.h>
#include <lamure/utils.h>

#include <GL/freeglut.h>

class split_screen_renderer
{
public:
                        split_screen_renderer(std::vector<scm::math::mat4f> const& model_transformations);
    virtual             ~split_screen_renderer();

    void                reset();

    void                render(lamure::context_t context_id, lamure::ren::Camera const& camera, const lamure::view_t view_id, const uint32_t target, scm::gl::vertex_array_ptr render_VAO);

    void                reset_viewport(int const x, int const y);

    void                send_model_transform(const lamure::model_t model_id, const scm::math::mat4f& transform);

    scm::gl::render_device_ptr device() const { return device_; }

protected:
    void                upload_transformation_matrices(lamure::ren::Camera const& camera, lamure::model_t model_id, uint32_t pass_id) const;

    bool                initialize_device_and_shaders(int resX, int resY);
    void                initialize_vbos();
    void                update_frustum_dependent_parameters(lamure::ren::Camera const& camera);

    void                upload_uniforms(lamure::ren::Camera const& camera) const;
    void                swap_temporary_buffers();
    void                display_status();

    void                calc_rad_scale_factors();
private:

    int                                         win_x_;
    int                                         win_y_;

    scm::shared_ptr<scm::gl::render_device>     device_;
    scm::shared_ptr<scm::gl::render_context>    context_;

    scm::gl::sampler_state_ptr                  filter_linear_;
    scm::gl::sampler_state_ptr                  filter_nearest_;
    scm::gl::blend_state_ptr                    color_blending_state_;

    scm::gl::rasterizer_state_ptr               change_point_size_in_shader_state_;

    scm::gl::depth_stencil_state_ptr            depth_state_less_;
    scm::gl::depth_stencil_state_ptr            depth_state_disable_;

    scm::gl::program_ptr                        pass1_visibility_shader_program_;
    scm::gl::program_ptr                        pass2_accumulation_shader_program_;
    scm::gl::program_ptr                        pass3_pass_trough_shader_program_;
    scm::gl::program_ptr                        pass_filling_program_;
    scm::gl::program_ptr                        bounding_box_vis_shader_program_;

    scm::gl::texture_2d_ptr                     gaussian_texture_;

    scm::gl::frame_buffer_ptr                   pass1_visibility_fbo_;
    scm::gl::texture_2d_ptr                     pass1_depth_buffer_;

    scm::gl::frame_buffer_ptr                   pass2_accumulation_fbo_;
    scm::gl::texture_2d_ptr                     pass2_accumulated_color_buffer_;
    scm::gl::texture_2d_ptr                     pass2_depth_buffer_;

    scm::gl::frame_buffer_ptr                   pass_filling_fbo_;
    scm::gl::texture_2d_ptr                     pass_filling_color_texture_;

    scm::gl::frame_buffer_ptr                   user_0_fbo_;
    scm::gl::texture_2d_ptr                     user_0_fbo_texture_;

    scm::gl::frame_buffer_ptr                   user_1_fbo_;
    scm::gl::texture_2d_ptr                     user_1_fbo_texture_;

    scm::gl::program_ptr                        render_target_shader_program_;


    scm::shared_ptr<scm::gl::quad_geometry>     screen_quad_A_;

    float                                       height_divided_by_top_minus_bottom_;  //is frustum dependent
    float                                       near_plane_;                          //is frustum dependent
    float                                       far_minus_near_plane_;

    float                                       point_size_factor_;

    //render setting state variables

    int                                         render_mode_;  //0 = color, 1 = normal, 2 = depth
    bool                                        ellipsify_; //true = elliptical, false = round

    bool                                        render_splats_;
    bool                                        render_bounding_boxes_;

    bool                                        clamped_normal_mode_; //true = clamp max ratio for of normals to max_deform_ratio_
    float                                       max_deform_ratio_;

    scm::gl::text_renderer_ptr                              text_renderer_;
    scm::gl::text_ptr                                       renderable_text_;
    scm::time::accum_timer<scm::time::high_res_timer>       frame_time_;
    double                                                  fps_;
    unsigned long long                                      rendered_splats_;
    unsigned long long                                      uploaded_nodes_;

    bool                                                    is_cut_update_active_;
    lamure::view_t                                                  current_cam_id_left_;
    lamure::view_t                                                  current_cam_id_right_;

    float const                                 resize_value;
    bool                                        display_info_;

    std::vector<scm::math::mat4f>               model_transformations_;
    std::vector<float>                          rad_scale_fac_;
//methods for changing rendering settings dynamically
public:

    void switch_bounding_box_rendering();
    void switch_surfel_rendering();
    void change_pointsize(float amount);
    void SwitchrenderMode();
    void SwitchEllipseMode();
    void SwitchClampedNormalMode();
    void ChangeDeformRatio(float amount);
    void SwitchLighting();
    void ToggleCutUpdateInfo();
    void ToggleCameraInfo(const lamure::view_t left_cam_id, const lamure::view_t right_cam_id);
    void ToggleDisplayInfo();
};


#endif // REN_SPLIT_SCREEN_RENDERER_H_

