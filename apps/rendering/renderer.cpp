// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "renderer.h"

#include <ctime>
#include <chrono>

#include <lamure/config.h>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <FreeImagePlus.h>

#include <scm/gl_core/render_device/opengl/gl_core.h>

Renderer::
Renderer(std::vector<scm::math::mat4f> const& model_transformations,
         const std::set<lamure::model_t>& visible_set,
         const std::set<lamure::model_t>& invisible_set)
    : near_plane_(0.f),
      far_plane_(1000.0f),
      elapsed_ms_since_cut_update_(0),
      visible_set_(visible_set),
      invisible_set_(invisible_set),
      render_visible_set_(true),
      point_size_factor_(1.0f),
      render_bounding_boxes_(false),
      fps_(0.0),
      rendered_splats_(0),
      is_cut_update_active_(true),
      current_cam_id_(0),
      display_info_(true),
#ifdef LAMURE_ENABLE_LINE_VISUALIZATION
      max_lines_(256),
#endif
      model_transformations_(model_transformations),
      radius_scale_(1.f)
{
    lamure::ren::ModelDatabase* database = lamure::ren::ModelDatabase::GetInstance();

    win_x_ = database->window_width();
    win_y_ = database->window_height();

    initialize_schism_device_and_shaders(win_x_, win_y_);
    initialize_VBOs();
    reset_viewport(win_x_, win_y_);

    calculate_radius_scale_per_model();
}

Renderer::
~Renderer()
{
    filter_nearest_.reset();
    color_blending_state_.reset();

    depth_state_disable_.reset();

    pass1_visibility_shader_program_.reset();
    pass2_accumulation_shader_program_.reset();
    pass3_pass_through_shader_program_.reset();

    bounding_box_vis_shader_program_.reset();

    pass1_depth_buffer_.reset();
    pass1_visibility_fbo_.reset();

    pass2_accumulated_color_buffer_.reset();

    pass2_accumulation_fbo_.reset();

    pass3_normalization_color_texture_.reset();
    pass3_normalization_normal_texture_.reset();

    screen_quad_.reset();

    context_.reset();
    device_.reset();
}

void Renderer::
upload_uniforms(lamure::ren::Camera const& camera) const
{
    using namespace lamure::ren;
    using namespace scm::gl;
    using namespace scm::math;

    pass1_visibility_shader_program_->uniform("near_plane", near_plane_);
    pass1_visibility_shader_program_->uniform("far_plane", far_plane_);
    pass1_visibility_shader_program_->uniform("point_size_factor", point_size_factor_);

    pass2_accumulation_shader_program_->uniform("near_plane", near_plane_);
    pass2_accumulation_shader_program_->uniform("far_plane", far_plane_ );
    pass2_accumulation_shader_program_->uniform("point_size_factor", point_size_factor_);

    pass3_pass_through_shader_program_->uniform_sampler("in_color_texture", 0);
    pass3_pass_through_shader_program_->uniform_sampler("in_normal_texture", 1);

    pass_filling_program_->uniform_sampler("in_color_texture", 0);
    pass_filling_program_->uniform_sampler("depth_texture", 1);
    pass_filling_program_->uniform("win_size", scm::math::vec2f(win_x_, win_y_) );


    context_->clear_default_color_buffer(FRAMEBUFFER_BACK, vec4f(0.0f, 0.0f, .0f, 1.0f)); // how does the image look, if nothing is drawn
    context_->clear_default_depth_stencil_buffer();

    context_->apply();
}

void Renderer::
upload_transformation_matrices(lamure::ren::Camera const& camera, lamure::model_t const model_id, RenderPass const pass) const {
    using namespace lamure::ren;

    scm::math::mat4f    model_matrix        = model_transformations_[model_id];
    scm::math::mat4f    projection_matrix   = camera.GetProjectionMatrix();

#if 1
    scm::math::mat4d    vm = camera.GetHighPrecisionViewMatrix();
    scm::math::mat4d    mm = scm::math::mat4d(model_matrix);
    scm::math::mat4d    vmd = vm * mm;
    
    scm::math::mat4f    model_view_matrix = scm::math::mat4f(vmd);

    scm::math::mat4d    mvpd = scm::math::mat4d(projection_matrix) * vmd;
    
#define DEFAULT_PRECISION 31
#else
    scm::math::mat4f    model_view_matrix   = view_matrix * model_matrix;
#endif

    float total_radius_scale = radius_scale_ * radius_scale_per_model_[model_id];

    switch(pass) {
        case RenderPass::DEPTH:
            pass1_visibility_shader_program_->uniform("mvp_matrix", scm::math::mat4f(mvpd) );
            pass1_visibility_shader_program_->uniform("model_view_matrix", model_view_matrix);
            pass1_visibility_shader_program_->uniform("inv_mv_matrix", scm::math::mat4f(scm::math::transpose(scm::math::inverse(vmd))));
            pass1_visibility_shader_program_->uniform("model_radius_scale", total_radius_scale);
            break;

        case RenderPass::ACCUMULATION:
            pass2_accumulation_shader_program_->uniform("mvp_matrix", scm::math::mat4f(mvpd));
            pass2_accumulation_shader_program_->uniform("model_view_matrix", model_view_matrix);
            pass2_accumulation_shader_program_->uniform("inv_mv_matrix", scm::math::mat4f(scm::math::transpose(scm::math::inverse(vmd))));
            pass2_accumulation_shader_program_->uniform("model_radius_scale", total_radius_scale);
            break;            

        case RenderPass::BOUNDING_BOX:
            bounding_box_vis_shader_program_->uniform("projection_matrix", projection_matrix);
            bounding_box_vis_shader_program_->uniform("model_view_matrix", model_view_matrix );
            break;

#ifdef LAMURE_ENABLE_LINE_VISUALIZATION
        case RenderPass::LINE:
            line_shader_program_->uniform("projection_matrix", projection_matrix);
            line_shader_program_->uniform("view_matrix", view_matrix );
            break;
#endif
        default:
            //LOGGER_ERROR("Unknown Pass ID used in function 'upload_transformation_matrices'");
            std::cout << "Unknown Pass ID used in function 'upload_transformation_matrices'\n";
            break;

    }

    context_->apply();
}


void Renderer::
render(lamure::context_t context_id, lamure::ren::Camera const& camera, const lamure::view_t view_id, scm::gl::vertex_array_ptr render_VAO, const unsigned current_camera_session)
{
    using namespace lamure;
    using namespace lamure::ren;

    update_frustum_dependent_parameters(camera);

    upload_uniforms(camera);

    using namespace scm::gl;
    using namespace scm::math;

    ModelDatabase* database = ModelDatabase::GetInstance();
    CutDatabase* cuts = CutDatabase::GetInstance();

    size_t NumbersOfSurfelsPerNode = database->surfels_per_node();
    model_t num_models = database->num_models();

    //determine set of models to render
    std::set<lamure::model_t> current_set;
    for (lamure::model_t model_id = 0; model_id < num_models; ++model_id) {
        auto vs_it = visible_set_.find(model_id);
        auto is_it = invisible_set_.find(model_id);

        if (vs_it == visible_set_.end() && is_it == invisible_set_.end()) {
            current_set.insert(model_id);
        }
        else if (vs_it != visible_set_.end()) {
            if (render_visible_set_) {
                current_set.insert(model_id);
            }
        }
        else if (is_it != invisible_set_.end()) {
            if (!render_visible_set_) {
                current_set.insert(model_id);
            }
        }

    }


    rendered_splats_ = 0;

    std::vector<uint32_t>                       frustum_culling_results;

    uint32_t size_of_culling_result_vector = 0;

    for (auto& model_id : current_set) {
        Cut& cut = cuts->GetCut(context_id, view_id, model_id);

        std::vector<Cut::NodeSlotAggregate> renderable = cut.complete_set();

        size_of_culling_result_vector += renderable.size();
    }

     frustum_culling_results.clear();
     frustum_culling_results.resize(size_of_culling_result_vector);

#ifdef LAMURE_RENDERING_ENABLE_PERFORMANCE_MEASUREMENT
     size_t depth_pass_time = 0;
     size_t accumulation_pass_time = 0;
     size_t normalization_pass_time = 0;
     size_t hole_filling_pass_time = 0;
#endif



            {


                /***************************************************************************************
                *******************************BEGIN DEPTH PASS*****************************************
                ****************************************************************************************/

                {
                    context_->clear_depth_stencil_buffer(pass1_visibility_fbo_);


                    context_->set_frame_buffer(pass1_visibility_fbo_);

                    context_->set_rasterizer_state(no_backface_culling_rasterizer_state_);
                    context_->set_viewport(viewport(vec2ui(0, 0), 1 * vec2ui(win_x_, win_y_)));

                    context_->bind_program(pass1_visibility_shader_program_);


                    context_->bind_vertex_array(render_VAO);
                    context_->apply();

                    node_t node_counter = 0;

                    for (auto& model_id : current_set) {
                        Cut& cut = cuts->GetCut(context_id, view_id, model_id);

                        std::vector<Cut::NodeSlotAggregate> renderable = cut.complete_set();

                        const Bvh* bvh = database->GetModel(model_id)->bvh();

                        size_t surfels_per_node_of_model = bvh->surfels_per_node();
                        //size_t surfels_per_node_of_model = NumbersOfSurfelsPerNode;
                        //store culling result and push it back for second pass#

                        std::vector<scm::gl::boxf>const & bounding_box_vector = bvh->bounding_boxes();


                        upload_transformation_matrices(camera, model_id, RenderPass::DEPTH);

                        scm::gl::frustum frustum_by_model = camera.GetFrustumByModel(model_transformations_[model_id]);


                        for(auto const& node_slot_aggregate : renderable) {
                            uint32_t node_culling_result = camera.CullAgainstFrustum( frustum_by_model ,bounding_box_vector[ node_slot_aggregate.node_id_ ] );


                             frustum_culling_results[node_counter] = node_culling_result;


                            if( (node_culling_result != 1) ) {
                                context_->apply();
#ifdef LAMURE_RENDERING_ENABLE_PERFORMANCE_MEASUREMENT
                                scm::gl::timer_query_ptr depth_pass_timer_query = device_->create_timer_query();
                                context_->begin_query(depth_pass_timer_query);
#endif

                                context_->draw_arrays(PRIMITIVE_POINT_LIST, (node_slot_aggregate.slot_id_) * NumbersOfSurfelsPerNode, surfels_per_node_of_model);

#ifdef LAMURE_RENDERING_ENABLE_PERFORMANCE_MEASUREMENT

                                context_->collect_query_results(depth_pass_timer_query);
                                depth_pass_time += depth_pass_timer_query->result();
#endif
                            }

                            ++node_counter;
                        }
                   }
                }



                /***************************************************************************************
                *******************************BEGIN ACCUMULATION PASS**********************************
                ****************************************************************************************/
                {

                    context_->clear_color_buffer(pass2_accumulation_fbo_ , 0, vec4f( .0f, .0f, .0f, 0.0f));
                    context_->clear_color_buffer(pass2_accumulation_fbo_ , 1, vec4f( .0f, .0f, .0f, 0.0f));

                    pass2_accumulation_fbo_->attach_depth_stencil_buffer(pass1_depth_buffer_);

                    context_->set_frame_buffer(pass2_accumulation_fbo_);

                    context_->set_rasterizer_state(no_backface_culling_rasterizer_state_);
                    context_->set_blend_state(color_blending_state_);

                    context_->set_depth_stencil_state(depth_state_test_without_writing_);

                    context_->bind_program(pass2_accumulation_shader_program_);

                    context_->bind_vertex_array(render_VAO);
                    context_->apply();

                   node_t node_counter = 0;

                   node_t actually_rendered_nodes = 0;


                    for (auto& model_id : current_set) {
                        Cut& cut = cuts->GetCut(context_id, view_id, model_id);

                        std::vector<Cut::NodeSlotAggregate> renderable = cut.complete_set();

                        const Bvh* bvh = database->GetModel(model_id)->bvh();

                        size_t surfels_per_node_of_model = bvh->surfels_per_node();


                        upload_transformation_matrices(camera, model_id, RenderPass::ACCUMULATION);

                        for( auto const& node_slot_aggregate : renderable ) {

                            if( frustum_culling_results[node_counter] != 1)  // 0 = inside, 1 = outside, 2 = intersectingS
                            {
                                context_->apply();

#ifdef LAMURE_RENDERING_ENABLE_PERFORMANCE_MEASUREMENT
                                scm::gl::timer_query_ptr accumulation_pass_timer_query = device_->create_timer_query();
                                context_->begin_query(accumulation_pass_timer_query);
#endif
                                context_->draw_arrays(PRIMITIVE_POINT_LIST, (node_slot_aggregate.slot_id_) * NumbersOfSurfelsPerNode, surfels_per_node_of_model);
#ifdef LAMURE_RENDERING_ENABLE_PERFORMANCE_MEASUREMENT
                                context_->end_query(accumulation_pass_timer_query);
                                context_->collect_query_results(accumulation_pass_timer_query);
                                accumulation_pass_time += accumulation_pass_timer_query->result();
#endif

                                ++actually_rendered_nodes;
                            }

                            ++node_counter;
                        }


                   }
                    rendered_splats_ = actually_rendered_nodes * database->surfels_per_node();

                }

                /***************************************************************************************
                *******************************BEGIN NORMALIZATION PASS*********************************
                ****************************************************************************************/


                {

                    //context_->set_default_frame_buffer();
                    context_->clear_color_buffer(pass3_normalization_fbo_, 0, vec4( 0.0, 0.0, 0.0, 0.0) );
                    context_->clear_color_buffer(pass3_normalization_fbo_, 1, vec3( 0.0, 0.0, 0.0) );

                    context_->set_frame_buffer(pass3_normalization_fbo_);

                    context_->set_depth_stencil_state(depth_state_disable_);

                    context_->bind_program(pass3_pass_through_shader_program_);

                    context_->bind_texture(pass2_accumulated_color_buffer_, filter_nearest_,   0);
                    context_->bind_texture(pass2_accumulated_normal_buffer_, filter_nearest_, 1);
                    context_->apply();

#ifdef LAMURE_RENDERING_ENABLE_PERFORMANCE_MEASUREMENT
                    scm::gl::timer_query_ptr normalization_pass_timer_query = device_->create_timer_query();
                    context_->begin_query(normalization_pass_timer_query);
#endif
                    screen_quad_->draw(context_);
#ifdef LAMURE_RENDERING_ENABLE_PERFORMANCE_MEASUREMENT
                    context_->end_query(normalization_pass_timer_query);
                    context_->collect_query_results(normalization_pass_timer_query);
                    normalization_pass_time += normalization_pass_timer_query->result();
#endif


                }

                /***************************************************************************************
                *******************************BEGIN RECONSTRUCTION PASS*********************************
                ****************************************************************************************/
                {
                    context_->set_default_frame_buffer();

                    context_->bind_program(pass_filling_program_);



                    context_->bind_texture(pass3_normalization_color_texture_, filter_nearest_,   0);
                    context_->bind_texture(pass1_depth_buffer_, filter_nearest_,   1);
                    context_->apply();

#ifdef LAMURE_RENDERING_ENABLE_PERFORMANCE_MEASUREMENT
                    scm::gl::timer_query_ptr hole_filling_pass_timer_query = device_->create_timer_query();
                    context_->begin_query(hole_filling_pass_timer_query);
#endif
                    screen_quad_->draw(context_);
#ifdef LAMURE_RENDERING_ENABLE_PERFORMANCE_MEASUREMENT
                    context_->end_query(hole_filling_pass_timer_query);
                    context_->collect_query_results(hole_filling_pass_timer_query);
                    hole_filling_pass_time += hole_filling_pass_timer_query->result();
#endif

                }




        if(render_bounding_boxes_)
        {

                        context_->set_default_frame_buffer();

                        context_->bind_program(bounding_box_vis_shader_program_);

                        context_->apply();



                    node_t node_counter = 0;

                    for (auto& model_id : current_set)
                    {
                        Cut& cut = cuts->GetCut(context_id, view_id, model_id);

                        std::vector<Cut::NodeSlotAggregate> renderable = cut.complete_set();


                        upload_transformation_matrices(camera, model_id, RenderPass::BOUNDING_BOX);

                        for( auto const& node_slot_aggregate : renderable ) {

                            int culling_result = frustum_culling_results[node_counter];

                            if( culling_result  != 1 )  // 0 = inside, 1 = outside, 2 = intersectingS
                            {

                                scm::gl::boxf temp_box = database->GetModel(model_id)->bvh()->bounding_boxes()[node_slot_aggregate.node_id_ ];
                                scm::gl::box_geometry box_to_render(device_,temp_box.min_vertex(), temp_box.max_vertex());




                                bounding_box_vis_shader_program_->uniform("culling_status", culling_result);


                                device_->opengl_api().glDisable(GL_DEPTH_TEST);
                                box_to_render.draw(context_, scm::gl::geometry::MODE_WIRE_FRAME);
                                device_->opengl_api().glEnable(GL_DEPTH_TEST);

                            }

                            ++node_counter;
                        }


                    }


        }


        context_->reset();
        frame_time_.stop();
        frame_time_.start();

        if (true)
        {
            //schism bug ? time::to_seconds yields milliseconds
            if (scm::time::to_seconds(frame_time_.accumulated_duration()) > 100.0)
            {

                fps_ = 1000.0f / scm::time::to_seconds(frame_time_.average_duration());


                frame_time_.reset();
            }
        }


#ifdef LAMURE_ENABLE_LINE_VISUALIZATION


    scm::math::vec3f* line_mem = (scm::math::vec3f*)device_->main_context()->map_buffer(line_buffer_, scm::gl::ACCESS_READ_WRITE);
    unsigned int num_valid_lines = 0;
    for (unsigned int i = 0; i < max_lines_; ++i) {
      if (i < line_begin_.size() && i < line_end_.size()) {
         line_mem[num_valid_lines*2+0] = line_begin_[i];
         line_mem[num_valid_lines*2+1] = line_end_[i];
         ++num_valid_lines; 
      }
    }

    device_->main_context()->unmap_buffer(line_buffer_);

    upload_transformation_matrices(camera, 0, LINE);
    device_->opengl_api().glDisable(GL_DEPTH_TEST);

    context_->set_default_frame_buffer();

    context_->bind_program(line_shader_program_);

    context_->bind_vertex_array(line_memory_);
    context_->apply();

    context_->draw_arrays(PRIMITIVE_LINE_LIST, 0, 2*num_valid_lines);


    device_->opengl_api().glEnable(GL_DEPTH_TEST);
#endif

    //if(display_info_)
      //display_status(current_camera_session);

    context_->reset();





    }

#ifdef LAMURE_RENDERING_ENABLE_PERFORMANCE_MEASUREMENT
uint64_t total_time = depth_pass_time + accumulation_pass_time + normalization_pass_time + hole_filling_pass_time;

std::cout << "depth pass        : " << depth_pass_time / ((float)(1000000)) << "ms (" << depth_pass_time        /( (float)(total_time) ) << ")\n"
          << "accumulation pass : " << accumulation_pass_time / ((float)(1000000)) << "ms (" << accumulation_pass_time /( (float)(total_time) )<< ")\n"
          << "normalization pass: " << normalization_pass_time / ((float)(1000000)) << "ms (" << normalization_pass_time/( (float)(total_time) )<< ")\n"
          << "hole filling  pass: " << hole_filling_pass_time / ((float)(1000000)) << "ms (" << hole_filling_pass_time /( (float)(total_time) )<< ")\n\n";

#endif

}


void Renderer::send_model_transform(const lamure::model_t model_id, const scm::math::mat4f& transform) {
    model_transformations_[model_id] = transform;
}

void Renderer::display_status(std::string const& information_to_display)
{
    std::stringstream os;
   // os.setprecision(5);
    os
      <<"FPS:   "<<std::setprecision(4)<<fps_<<"\n"
      <<"# Points:   "<< (rendered_splats_ / 100000) / 10.0f<< " Mio. \n";
      
    os << information_to_display;
    os << "\n";
    
    renderable_text_->text_string(os.str());
    text_renderer_->draw_shadowed(context_, scm::math::vec2i(20, win_y_- 40), renderable_text_);
}

void Renderer::
initialize_VBOs()
{
    // init the GL context
    using namespace scm;
    using namespace scm::gl;
    using namespace scm::math;


    filter_nearest_ = device_->create_sampler_state(FILTER_MIN_MAG_LINEAR, WRAP_CLAMP_TO_EDGE);

    no_backface_culling_rasterizer_state_ = device_->create_rasterizer_state(FILL_SOLID, CULL_NONE, ORIENT_CCW, false, false, 0.0, false, false);

    pass1_visibility_fbo_ = device_->create_frame_buffer();

    pass1_depth_buffer_           = device_->create_texture_2d(vec2ui(win_x_, win_y_) * 1, FORMAT_D32F, 1, 1, 1);

    pass1_visibility_fbo_->attach_depth_stencil_buffer(pass1_depth_buffer_);



    pass2_accumulation_fbo_ = device_->create_frame_buffer();

    pass2_accumulated_color_buffer_   = device_->create_texture_2d(vec2ui(win_x_, win_y_) * 1, FORMAT_RGBA_32F , 1, 1, 1);

    pass2_accumulation_fbo_->attach_color_buffer(0, pass2_accumulated_color_buffer_);

    pass2_accumulated_normal_buffer_   = device_->create_texture_2d(vec2ui(win_x_, win_y_) * 1, FORMAT_RGB_32F , 1, 1, 1);

    pass2_accumulation_fbo_->attach_color_buffer(1, pass2_accumulated_normal_buffer_);

    pass2_accumulation_fbo_->attach_depth_stencil_buffer(pass1_depth_buffer_);


    pass3_normalization_fbo_ = device_->create_frame_buffer();

    pass3_normalization_color_texture_ = device_->create_texture_2d(scm::math::vec2ui(win_x_, win_y_) * 1, scm::gl::FORMAT_RGBA_8 , 1, 1, 1);
    pass3_normalization_normal_texture_ = device_->create_texture_2d(scm::math::vec2ui(win_x_, win_y_) * 1, scm::gl::FORMAT_RGB_8 , 1, 1, 1);

    pass3_normalization_fbo_->attach_color_buffer(0, pass3_normalization_color_texture_);
    pass3_normalization_fbo_->attach_color_buffer(1, pass3_normalization_normal_texture_);


    screen_quad_.reset(new quad_geometry(device_, vec2f(-1.0f, -1.0f), vec2f(1.0f, 1.0f)));



    color_blending_state_ = device_->create_blend_state(true, FUNC_ONE, FUNC_ONE, FUNC_ONE, FUNC_ONE, EQ_FUNC_ADD, EQ_FUNC_ADD);


    depth_state_disable_ = device_->create_depth_stencil_state(false, true, scm::gl::COMPARISON_NEVER);

    depth_state_test_without_writing_ = device_->create_depth_stencil_state(true, false, scm::gl::COMPARISON_LESS_EQUAL);

#ifdef LAMURE_ENABLE_LINE_VISUALIZATION
    std::size_t size_of_line_buffer = max_lines_ * sizeof(float) * 3 * 2;
    line_buffer_ = device_->create_buffer(scm::gl::BIND_VERTEX_BUFFER,
                                    scm::gl::USAGE_DYNAMIC_DRAW,
                                    size_of_line_buffer,
                                    0);
    line_memory_ = device_->create_vertex_array(scm::gl::vertex_format
            (0, 0, scm::gl::TYPE_VEC3F, sizeof(float)*3),
            boost::assign::list_of(line_buffer_));

#endif
}

bool Renderer::
initialize_schism_device_and_shaders(int resX, int resY)
{
    std::string root_path = LAMURE_SHADERS_DIR;

    std::string visibility_vs_source;
    std::string visibility_gs_source;
    std::string visibility_fs_source;

    std::string pass_trough_vs_source;
    std::string pass_trough_fs_source;

    std::string accumulation_vs_source;
    std::string accumulation_gs_source;
    std::string accumulation_fs_source;

    std::string filling_vs_source;
    std::string filling_fs_source;

    std::string bounding_box_vs_source;
    std::string bounding_box_fs_source;

#ifdef LAMURE_ENABLE_LINE_VISUALIZATION
    std::string line_vs_source;
    std::string line_fs_source;
#endif
    try {

        using scm::io::read_text_file;

        if (!read_text_file(root_path +  "/pass1_visibility_pass.glslv", visibility_vs_source)
            || !read_text_file(root_path + "/pass1_visibility_pass.glslg", visibility_gs_source)
            || !read_text_file(root_path + "/pass1_visibility_pass.glslf", visibility_fs_source)
            || !read_text_file(root_path + "/pass2_accumulation_pass.glslv", accumulation_vs_source)
            || !read_text_file(root_path + "/pass2_accumulation_pass.glslg", accumulation_gs_source)
            || !read_text_file(root_path + "/pass2_accumulation_pass.glslf", accumulation_fs_source)
            || !read_text_file(root_path + "/pass3_pass_through.glslv", pass_trough_vs_source)
            || !read_text_file(root_path + "/pass3_pass_through.glslf", pass_trough_fs_source)
            || !read_text_file(root_path + "/pass_reconstruction.glslv", filling_vs_source)
            || !read_text_file(root_path + "/pass_reconstruction.glslf", filling_fs_source)
            || !read_text_file(root_path + "/bounding_box_vis.glslv", bounding_box_vs_source)
            || !read_text_file(root_path + "/bounding_box_vis.glslf", bounding_box_fs_source)
#ifdef LAMURE_ENABLE_LINE_VISUALIZATION
            || !read_text_file(root_path + "/lines_shader.glslv", line_vs_source)
            || !read_text_file(root_path + "/lines_shader.glslf", line_fs_source)
#endif
           )
           {
               scm::err() << "error reading shader files" << scm::log::end;
               return false;
           }
    }
    catch (std::exception& e)
    {

        std::cout << e.what() << std::endl;
    }


    device_.reset(new scm::gl::render_device());

    context_ = device_->main_context();

    scm::out() << *device_ << scm::log::end;

    //using namespace boost::assign;
    pass1_visibility_shader_program_ = device_->create_program(
                                                  boost::assign::list_of(device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, visibility_vs_source))
                                                                        (device_->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, visibility_gs_source))
                                                                        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, visibility_fs_source))
                                                              );

    pass2_accumulation_shader_program_ = device_->create_program(
                                                    boost::assign::list_of(device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, accumulation_vs_source))
                                                                          (device_->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, accumulation_gs_source))
                                                                          (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER,accumulation_fs_source))
                                                                );

    pass3_pass_through_shader_program_ = device_->create_program(boost::assign::list_of(device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, pass_trough_vs_source))
                                                                (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, pass_trough_fs_source)));

    pass_filling_program_ = device_->create_program(boost::assign::list_of(device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, filling_vs_source))
                                                    (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, filling_fs_source)));

    bounding_box_vis_shader_program_ = device_->create_program(boost::assign::list_of(device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, bounding_box_vs_source))
                                                               (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, bounding_box_fs_source)));

#ifdef LAMURE_ENABLE_LINE_VISUALIZATION
    line_shader_program_ = device_->create_program(boost::assign::list_of(device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, line_vs_source))
                                                   (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, line_fs_source)));
#endif


    if (!pass1_visibility_shader_program_ || !pass2_accumulation_shader_program_ || !pass3_pass_through_shader_program_ || !pass_filling_program_ || !bounding_box_vis_shader_program_

#ifdef LAMURE_ENABLE_LINE_VISUALIZATION
        || !line_shader_program_
#endif
       ) {
        scm::err() << "error creating shader programs" << scm::log::end;
        return false;
    }


    scm::out() << *device_ << scm::log::end;


    using namespace scm;
    using namespace scm::gl;
    using namespace scm::math;

    try {
        font_face_ptr output_font(new font_face(device_, std::string(LAMURE_FONTS_DIR) + "/Ubuntu.ttf", 30, 0, font_face::smooth_lcd));
        text_renderer_  =     scm::make_shared<text_renderer>(device_);
        renderable_text_    = scm::make_shared<scm::gl::text>(device_, output_font, font_face::style_regular, "sick, sad world...");

        mat4f   fs_projection = make_ortho_matrix(0.0f, static_cast<float>(win_x_),
                                                  0.0f, static_cast<float>(win_y_), -1.0f, 1.0f);
        text_renderer_->projection_matrix(fs_projection);

        renderable_text_->text_color(math::vec4f(1.0f, 1.0f, 1.0f, 1.0f));
        renderable_text_->text_kerning(true);
    }
    catch(const std::exception& e) {
        throw std::runtime_error(std::string("vtexture_system::vtexture_system(): ") + e.what());
    }

    return true;
}

void Renderer::reset_viewport(int w, int h)
{
    //reset viewport
    win_x_ = w;
    win_y_ = h;
    context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(w, h)));


    //reset frame buffers and textures
    pass1_visibility_fbo_ = device_->create_frame_buffer();

    pass1_depth_buffer_           =device_->create_texture_2d(scm::math::vec2ui(win_x_, win_y_) * 1, scm::gl::FORMAT_D24, 1, 1, 1);

    pass1_visibility_fbo_->attach_depth_stencil_buffer(pass1_depth_buffer_);


    pass2_accumulation_fbo_ = device_->create_frame_buffer();

    pass2_accumulated_color_buffer_   = device_->create_texture_2d(scm::math::vec2ui(win_x_, win_y_) * 1, scm::gl::FORMAT_RGBA_32F , 1, 1, 1);
    pass2_accumulated_normal_buffer_   = device_->create_texture_2d(scm::math::vec2ui(win_x_, win_y_) * 1, scm::gl::FORMAT_RGB_32F , 1, 1, 1);
    
    pass2_accumulation_fbo_->attach_color_buffer(0, pass2_accumulated_color_buffer_);
    pass2_accumulation_fbo_->attach_color_buffer(1, pass2_accumulated_normal_buffer_);


    pass3_normalization_fbo_ = device_->create_frame_buffer();

    pass3_normalization_color_texture_ = device_->create_texture_2d(scm::math::vec2ui(win_x_, win_y_) * 1, scm::gl::FORMAT_RGBA_8 , 1, 1, 1);
    pass3_normalization_normal_texture_ = device_->create_texture_2d(scm::math::vec2ui(win_x_, win_y_) * 1, scm::gl::FORMAT_RGB_8 , 1, 1, 1);

    pass3_normalization_fbo_->attach_color_buffer(0, pass3_normalization_color_texture_);
    pass3_normalization_fbo_->attach_color_buffer(1, pass3_normalization_normal_texture_);


    //reset orthogonal projection matrix for text rendering
    scm::math::mat4f   fs_projection = scm::math::make_ortho_matrix(0.0f, static_cast<float>(win_x_),
                                                                    0.0f, static_cast<float>(win_y_), -1.0f, 1.0f);
    text_renderer_->projection_matrix(fs_projection);

}

void Renderer::
update_frustum_dependent_parameters(lamure::ren::Camera const& camera)
{
    near_plane_ = camera.near_plane_value();
    far_plane_  = camera.far_plane_value();

    std::vector<scm::math::vec3d> corner_values = camera.get_frustum_corners();
    double top_minus_bottom = scm::math::length((corner_values[2]) - (corner_values[0]));

    height_divided_by_top_minus_bottom_ = win_y_ / top_minus_bottom;
}

void Renderer::
calculate_radius_scale_per_model()
{
    using namespace lamure::ren;
    uint32_t num_models = (ModelDatabase::GetInstance())->num_models();

    if(radius_scale_per_model_.size() < num_models)
      radius_scale_per_model_.resize(num_models);

    scm::math::vec4f x_unit_vec = scm::math::vec4f(1.0,0.0,0.0,0.0);
    for(unsigned int model_id = 0; model_id < num_models; ++model_id)
    {
     radius_scale_per_model_[model_id] = scm::math::length(model_transformations_[model_id] * x_unit_vec);
    }
}

//dynamic rendering adjustment functions
void Renderer::
toggle_bounding_box_rendering()
{
    render_bounding_boxes_ = ! render_bounding_boxes_;

    std::cout<<"bounding box visualisation: ";
    if(render_bounding_boxes_)
        std::cout<<"ON\n\n";
    else
        std::cout<<"OFF\n\n";
};



void Renderer::
change_point_size(float amount)
{
    point_size_factor_ += amount;
    if(point_size_factor_ < 0.0001f)
    {
        point_size_factor_ = 0.0001;
    }

    std::cout<<"set point size factor to: "<<point_size_factor_<<"\n\n";
};

void Renderer::
toggle_cut_update_info() {
    is_cut_update_active_ = ! is_cut_update_active_;
}

void Renderer::
toggle_camera_info(const lamure::view_t current_cam_id) {
    current_cam_id_ = current_cam_id;
}

void Renderer::
toggle_display_info() {
    display_info_ = ! display_info_;
}

void Renderer::
toggle_visible_set() {
    render_visible_set_ = !render_visible_set_;
}

void Renderer::
take_screenshot(std::string const& screenshot_path, std::string const& screenshot_name) {

    std::string file_extension = ".png";
    {

        {
            if(! boost::filesystem::exists(screenshot_path+"/")) {
               std::cout<<"Screenshot Folder did not exist. Creating Folder: " << screenshot_path << "\n\n";
               boost::filesystem::create_directories(screenshot_path+"/");
            }
        }

        

        // Make the BYTE array, factor of 3 because it's RBG.
        BYTE* pixels = new BYTE[ 3 * win_x_ * win_y_];

        
        device_->opengl_api().glBindTexture(GL_TEXTURE_2D, pass3_normalization_color_texture_->object_id());
        device_->opengl_api().glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, pixels);

        std::string filename = screenshot_path + screenshot_name +"color_"  + file_extension;

        FIBITMAP* image = FreeImage_ConvertFromRawBits(pixels, win_x_, win_y_, 3 * win_x_, 24, 0x0000FF, 0xFF0000, 0x00FF00, false);
        FreeImage_Save(FIF_PNG, image, filename.c_str(), 0);


        device_->opengl_api().glBindTexture(GL_TEXTURE_2D, pass3_normalization_normal_texture_->object_id());
        device_->opengl_api().glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, pixels);

        filename = screenshot_path + "normal_" + screenshot_name + file_extension;

        image = FreeImage_ConvertFromRawBits(pixels, win_x_, win_y_, 3 * win_x_, 24, 0x0000FF, 0xFF0000, 0x00FF00, false);
        FreeImage_Save(FIF_PNG, image, filename.c_str(), 0);

        device_->opengl_api().glBindTexture(GL_TEXTURE_2D, 0);

        // Free resources
        FreeImage_Unload(image);
        delete [] pixels;

        std::cout<<"Saved Screenshot: "<<filename.c_str()<<"\n\n";
    }
}