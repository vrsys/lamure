// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_OLD_RENDERER_H_
#define REN_OLD_RENDERER_H_

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


#include <lamure/ren/ray.h>
//#define LAMURE_RENDERING_ENABLE_PERFORMANCE_MEASUREMENT


enum class RenderPass {
    DEPTH              = 0,
    ACCUMULATION       = 1,
    NORMALIZATION      = 2,
    BOUNDING_BOX       = 99,
    LINE               = 1111  
};

class Renderer
{
public:
                        Renderer(std::vector<scm::math::mat4f> const& model_transformations,
                            const std::set<lamure::model_t>& visible_set,
                            const std::set<lamure::model_t>& invisible_set);
    virtual             ~Renderer();

    //char*               GetMappedTempBufferPtr(CutDatabaseRecord::TemporaryBuffer const& buffer);
    //void                UnmapTempBuffer(CutDatabaseRecord::TemporaryBuffer const&  buffer);
    //void                CopyTempToMainMemory(context_t context_id, CutDatabaseRecord::TemporaryBuffer const& buffer);

    void                render(lamure::context_t context_id, lamure::ren::Camera const& camera, const lamure::view_t view_id, scm::gl::vertex_array_ptr render_VAO, const unsigned current_camera_session);

    void                reset_viewport(int const x, int const y);

    void                send_model_transform(const lamure::model_t model_id, const scm::math::mat4f& transform);

    void                set_radius_scale(const float radius_scale) { radius_scale_ = radius_scale; };


    scm::gl::render_device_ptr device() const { return device_; }

protected:
    bool                initialize_schism_device_and_shaders(int resX, int resY);
    void                initialize_VBOs();
    void                update_frustum_dependent_parameters(lamure::ren::Camera const& camera);

    void                upload_uniforms(lamure::ren::Camera const& camera) const;
    void                upload_transformation_matrices(lamure::ren::Camera const& camera, lamure::model_t const model_id, RenderPass const pass_id) const;
    void                swap_temp_buffers();
    void                display_status(unsigned const current_camera_session);

    void                calc_rad_scale_factors();
private:

        int                                         win_x_;
        int                                         win_y_;

        scm::shared_ptr<scm::gl::render_device>     device_;
        scm::shared_ptr<scm::gl::render_context>    context_;

        scm::gl::rasterizer_state_ptr               change_point_size_in_shader_state_;
        scm::gl::sampler_state_ptr                  filter_nearest_;
        scm::gl::blend_state_ptr                    color_blending_state_;
        scm::gl::blend_state_ptr                    not_blended_state_;

        scm::gl::depth_stencil_state_ptr            depth_state_less_;
        scm::gl::depth_stencil_state_ptr            depth_state_disable_;
        scm::gl::depth_stencil_state_ptr            depth_state_test_without_writing_;

        //shader programs
        scm::gl::program_ptr                        pass1_visibility_shader_program_;
        scm::gl::program_ptr                        pass2_accumulation_shader_program_;
        scm::gl::program_ptr                        pass3_pass_trough_shader_program_;
        scm::gl::program_ptr                        pass_filling_program_;
        scm::gl::program_ptr                        bounding_box_vis_shader_program_;

        //gaussian weight for smooth blending
        scm::gl::texture_2d_ptr                     gaussian_texture_;

        //framebuffer and textures for different passes
        scm::gl::frame_buffer_ptr                   pass1_visibility_fbo_;
        scm::gl::texture_2d_ptr                     pass1_depth_buffer_;

        scm::gl::frame_buffer_ptr                   pass2_accumulation_fbo_;
        scm::gl::texture_2d_ptr                     pass2_accumulated_color_buffer_;
        scm::gl::texture_2d_ptr                     pass2_depth_buffer_;

        scm::gl::frame_buffer_ptr                   pass_filling_fbo_;
        scm::gl::texture_2d_ptr                     pass_filling_color_texture_;


        scm::shared_ptr<scm::gl::quad_geometry>     screen_quad_;


        float                                       height_divided_by_top_minus_bottom_;  //frustum dependent
        float                                       near_plane_;                          //frustum dependent
        float                                       far_plane_;   
        float                                       point_size_factor_;


        //render setting state variables

        int                                         render_mode_;           //enable/disable filled holes Visualization
        bool                                        render_bounding_boxes_;

        //variables related to text rendering
        scm::gl::text_renderer_ptr                              text_renderer_;
        scm::gl::text_ptr                                       renderable_text_;
        scm::time::accum_timer<scm::time::high_res_timer>       frame_time_;
        double                                                  fps_;
        unsigned long long                                      rendered_splats_;
        unsigned long long                                      uploaded_nodes_;
        bool                                                    is_cut_update_active_;
        lamure::view_t                                          current_cam_id_;


        bool                                                    display_info_;

        std::vector<scm::math::mat4f>                           model_transformations_;
        std::vector<float>                                      rad_scale_fac_;
        float                                                   radius_scale_;

        static uint64_t                                         current_screenshot_num_;

        size_t                                                  elapsed_ms_since_cut_update_;

        std::set<lamure::model_t> visible_set_;
        std::set<lamure::model_t> invisible_set_;
        bool render_visible_set_;

        scm::gl::vertex_array_ptr line_memory_;
        scm::gl::buffer_ptr line_buffer_;
        scm::gl::program_ptr line_shader_program_;
        std::vector<scm::math::vec3f> line_begin_;
        std::vector<scm::math::vec3f> line_end_;
        unsigned int max_lines_;

//methods for changing rendering settings dynamically
public:
    void add_line_begin(const scm::math::vec3f& line_begin) { line_begin_.push_back(line_begin); };
    void add_line_end(const scm::math::vec3f& line_end) { line_end_.push_back(line_end); };
    void clear_line_begin() { line_begin_.clear(); };
    void clear_line_end() { line_end_.clear(); };

    void switch_bounding_box_rendering();
    void change_point_size(float amount);
    void switch_render_mode();
    void toggle_cut_update_info();
    void toggle_camera_info(const lamure::view_t current_cam_id);
    void take_screenshot();
    void toggle_visible_set();
    void toggle_display_info();
};

#endif // REN_OLD_RENDERER_H_
