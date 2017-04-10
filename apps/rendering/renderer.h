// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_OLD_RENDERER_H_
#define REN_OLD_RENDERER_H_

#include <lamure/lod/camera.h>
#include <lamure/lod/cut.h>
#include <lamure/lod/controller.h>
#include <lamure/lod/model_database.h>
#include <lamure/lod/cut_database.h>
#include <lamure/lod/controller.h>
#include <lamure/lod/policy.h>
#include <lamure/gl/shader.h>
#include <lamure/gl.h>

#include <memory>
#include <fstream>
#include <vector>
#include <map>
#include <lamure/types.h>

//#define LAMURE_RENDERING_ENABLE_PERFORMANCE_MEASUREMENT


enum class RenderPass {
    DEPTH                    = 0,
    ACCUMULATION             = 1,
    NORMALIZATION            = 2,
    LINKED_LIST_ACCUMULATION = 4,
    ONE_PASS_LQ              = 5,
    BOUNDING_BOX             = 100,
    LINE                     = 101,
    TRIMESH                  = 300
};

enum class RenderMode {
    HQ_ONE_PASS = 1,
    HQ_TWO_PASS = 2,
    LQ_ONE_PASS = 3
};


class Renderer
{
public:
                        Renderer(std::vector<lamure::mat4r_t> const& model_transformations,
                            const std::set<lamure::model_t>& visible_set,
                            const std::set<lamure::model_t>& invisible_set);
    virtual             ~Renderer();

    void                render(
		          lamure::context_t context_id, 
			  lamure::lod::camera const& camera, 
			  const lamure::view_t view_id, 
			  lamure::gl::vertex_array_t* render_VA, 
			  lamure::gl::array_buffer_t* render_AB, 
			  const unsigned current_camera_session);

    void                reset_viewport(int const x, int const y);

    void                send_model_transform(const lamure::model_t model_id, const lamure::mat4r_t& transform);

    void                set_radius_scale(const float radius_scale) { radius_scale_ = radius_scale; };

    void                switch_render_mode(RenderMode const& render_mode);

    void                display_status(std::string const& information_to_display);

    

protected:
    bool                initialize_schism_device_and_shaders(int resX, int resY);
    void                initialize_VBOs();
    void                update_frustum_dependent_parameters(lamure::lod::camera const& camera);

    //void                bind_storage_buffer(lamure::gl::array_buffer_t* buffer);

    void                upload_uniforms(lamure::lod::camera const& camera) const;
    void                upload_transformation_matrices(lamure::lod::camera const& camera, lamure::model_t const model_id, RenderPass const pass_id) const;
    void                swap_temp_buffers();
    void                calculate_radius_scale_per_model();


    void                render_one_pass_LQ(lamure::context_t context_id, 
                                           lamure::lod::camera const& camera, 
                                           const lamure::view_t view_id, 
                                           lamure::gl::vertex_array_t* render_VA,
					                                 lamure::gl::array_buffer_t* render_AB,
                                           std::set<lamure::model_t> const& current_set, std::vector<uint32_t>& frustum_culling_results);
/*
    void                render_two_pass_HQ(lamure::context_t context_id, 
                                           lamure::lod::camera const& camera, 
                                           const lamure::view_t view_id, 
                                           lamure::gl::vertex_array_t* render_VA,
					                                 lamure::gl::array_buffer_t* render_AB, 
                                           std::set<lamure::model_t> const&  current_set,
                                           std::vector<uint32_t>& frustum_culling_results);
*/
private:

        int                                         win_x_;
        int                                         win_y_;

	/*
        lamure::shared_ptr<lamure::gl::render_device>     device_;
        lamure::shared_ptr<lamure::gl::render_context>    context_;

        lamure::gl::sampler_state_ptr                  filter_nearest_;
        lamure::gl::blend_state_ptr                    color_blending_state_;

        lamure::gl::depth_stencil_state_ptr            depth_state_disable_;
        lamure::gl::depth_stencil_state_ptr            depth_state_test_without_writing_;

        lamure::gl::rasterizer_state_ptr               no_backface_culling_rasterizer_state_;
*/

        //shader programs
        lamure::gl::shader_t*                        LQ_one_pass_program_;
        lamure::gl::shader_t*                        bounding_box_vis_shader_program_;
        
/*
        lamure::gl::shader_t*                        pass1_visibility_shader_program_;
        lamure::gl::shader_t*                        pass2_accumulation_shader_program_;
        lamure::gl::shader_t*                        pass3_pass_through_shader_program_;
        lamure::gl::shader_t*                        pass_filling_program_;

	      lamure::gl::shader_t*                        pass1_linked_list_accumulate_program_;
	      lamure::gl::shader_t*                        pass2_linked_list_resolve_program_;
	      lamure::gl::shader_t*                        pass3_repair_program_;

        //framebuffer and textures for different passes
        lamure::gl::frame_buffer_t*                   pass1_visibility_fbo_;
        lamure::gl::texture_2d_t*                     pass1_depth_buffer_;

        lamure::gl::frame_buffer_t*                   pass2_accumulation_fbo_;
        lamure::gl::texture_2d_t*                     pass2_accumulated_color_buffer_;
        lamure::gl::texture_2d_t*                     pass2_accumulated_normal_buffer_;

        lamure::gl::frame_buffer_t*                   pass3_normalization_fbo_;
        lamure::gl::texture_2d_t*                     pass3_normalization_color_texture_;
        lamure::gl::texture_2d_t*                     pass3_normalization_normal_texture_;

	      lamure::gl::texture_2d_t*                     min_es_distance_image_;
	      lamure::gl::frame_buffer_t*                   atomic_image_fbo_;
	      lamure::gl::texture_2d_t*                     atomic_fragment_count_image_;
	      lamure::gl::texture_buffer_t*                 linked_list_buffer_texture_;
*/	
        float                                       height_divided_by_top_minus_bottom_;  //frustum dependent
        float                                       near_plane_;                          //frustum dependent
        float                                       far_plane_;   
        float                                       point_size_factor_;

	      float                                       blending_threshold_;

        bool                                        render_bounding_boxes_;

        //variables related to text rendering
        //lamure::gl::text_renderer_ptr                              text_renderer_;
        //lamure::gl::text_ptr                                       renderable_text_;
        //lamure::time::accum_timer<lamure::time::high_res_timer>       frame_time_;
        double                                                  fps_;
        unsigned long long                                      rendered_splats_;
        bool                                                    is_cut_update_active_;
        lamure::view_t                                          current_cam_id_;


        bool                                                    display_info_;

        std::vector<lamure::mat4r_t>                            model_transformations_;
        std::vector<float>                                      radius_scale_per_model_;
        float                                                   radius_scale_;

        size_t                                                  elapsed_ms_since_cut_update_;

        RenderMode                                              render_mode_;

        std::set<lamure::model_t> visible_set_;
        std::set<lamure::model_t> invisible_set_;
        bool render_visible_set_;

        lamure::gl::shader_t* trimesh_shader_program_;
        std::vector<lamure::vec3r_t> line_begin_;
        std::vector<lamure::vec3r_t> line_end_;
        unsigned int max_lines_;

//methods for changing rendering settings dynamically
public:
    void add_line_begin(const lamure::vec3r_t& line_begin) { line_begin_.push_back(line_begin); };
    void add_line_end(const lamure::vec3r_t& line_end) { line_end_.push_back(line_end); };
    void clear_line_begin() { line_begin_.clear(); };
    void clear_line_end() { line_end_.clear(); };

    void toggle_bounding_box_rendering();
    void change_point_size(float amount);
    void toggle_cut_update_info();
    void toggle_camera_info(const lamure::view_t current_cam_id);
    void take_screenshot(std::string const& screenshot_path, std::string const& screenshot_name);
    void toggle_visible_set();
    void toggle_display_info();
};

#endif // REN_OLD_RENDERER_H_
