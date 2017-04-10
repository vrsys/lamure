// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "renderer.h"

#include <ctime>
#include <chrono>

#include <lamure/lod/config.h>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <FreeImagePlus.h>

#define NUM_BLENDED_FRAGS 18

Renderer::
Renderer(std::vector<lamure::mat4r_t> const& model_transformations,
         const std::set<lamure::model_t>& visible_set,
         const std::set<lamure::model_t>& invisible_set)
    : near_plane_(0.f),
      far_plane_(1000.0f),
      point_size_factor_(1.0f),
      blending_threshold_(0.01f),
      render_bounding_boxes_(false),
      elapsed_ms_since_cut_update_(0),
      render_mode_(RenderMode::LQ_ONE_PASS),
      visible_set_(visible_set),
      invisible_set_(invisible_set),
      render_visible_set_(true),
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

    lamure::lod::policy* policy = lamure::lod::policy::get_instance();
    win_x_ = policy->window_width();
    win_y_ = policy->window_height();

    win_x_ = 800;
    win_y_ = 600;
    initialize_schism_device_and_shaders(win_x_, win_y_);
    initialize_VBOs();
    reset_viewport(win_x_, win_y_);

    calculate_radius_scale_per_model();
}

Renderer::
~Renderer()
{
 //TODO! free resources

}

void Renderer::
upload_uniforms(lamure::lod::camera const& camera) const
{
    using namespace lamure::lod;
    using namespace lamure::gl;

    model_database* database = model_database::get_instance();
    uint32_t number_of_surfels_per_node = database->get_primitives_per_node();
    unsigned num_blend_f = NUM_BLENDED_FRAGS;

    LQ_one_pass_program_->set("near_plane", near_plane_);
    LQ_one_pass_program_->set("far_plane", far_plane_);
    LQ_one_pass_program_->set("point_size_factor", point_size_factor_);

    glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

}

void Renderer::
upload_transformation_matrices(lamure::lod::camera const& camera, lamure::model_t const model_id, RenderPass const pass) const {
    using namespace lamure::lod;

    lamure::mat4r_t    model_matrix        = model_transformations_[model_id];
    lamure::mat4r_t    projection_matrix   = camera.get_projection_matrix();

#if 1
    lamure::mat4r_t    vm = camera.get_high_precision_view_matrix();
    lamure::mat4r_t    mm = lamure::mat4r_t(model_matrix);
    lamure::mat4r_t    vmd = vm * mm;

    lamure::mat4r_t    model_view_matrix = lamure::mat4r_t(vmd);

    lamure::mat4r_t    mvpd = lamure::mat4r_t(projection_matrix) * vmd;

#define DEFAULT_PRECISION 31
#else
    lamure::mat4r_t    model_view_matrix   = view_matrix * model_matrix;
#endif

    float total_radius_scale = radius_scale_;// * radius_scale_per_model_[model_id];

    switch(pass) {
    
        case RenderPass::ONE_PASS_LQ:
            LQ_one_pass_program_->set("mvp_matrix", lamure::mat4f_t(mvpd));
            LQ_one_pass_program_->set("model_view_matrix", lamure::mat4f_t(model_view_matrix));
            LQ_one_pass_program_->set("inv_mv_matrix", lamure::mat4f_t(lamure::math::transpose(lamure::math::inverse(vmd))));
            LQ_one_pass_program_->set("model_radius_scale", total_radius_scale);
            LQ_one_pass_program_->set("projection_matrix", lamure::mat4f_t(projection_matrix));
        break;

        case RenderPass::BOUNDING_BOX:
            bounding_box_vis_shader_program_->set("projection_matrix", lamure::mat4f_t(projection_matrix));
            bounding_box_vis_shader_program_->set("model_view_matrix", lamure::mat4f_t(model_view_matrix));
            break;

#ifdef LAMURE_ENABLE_LINE_VISUALIZATION
        case RenderPass::LINE:
            line_shader_program_->set("projection_matrix", lamure::mat4f_t(projection_matrix));
            line_shader_program_->set("view_matrix", lamure::mat4f_t(view_matrix));
            break;
#endif
        case RenderPass::TRIMESH:
            trimesh_shader_program_->set("mvp_matrix", lamure::mat4f_t(mvpd));
            break;

        default:
            //LOGGER_ERROR("Unknown Pass ID used in function 'upload_transformation_matrices'");
            std::cout << "Unknown Pass ID used in function 'upload_transformation_matrices'\n";
            break;

    }

}

void Renderer::
render_one_pass_LQ(lamure::context_t context_id,
                   lamure::lod::camera const& camera,
                   const lamure::view_t view_id,
                   lamure::gl::vertex_array_t* render_VA,
                   lamure::gl::array_buffer_t* render_AB,
                   std::set<lamure::model_t> const& current_set,
                   std::vector<uint32_t>& frustum_culling_results) {

    using namespace lamure;
    using namespace lamure::lod;

    using namespace lamure::gl;
    using namespace lamure::math;

    cut_database* cuts = cut_database::get_instance();
    model_database* database = model_database::get_instance();

    size_t number_of_surfels_per_node = database->get_primitives_per_node();

    /***************************************************************************************
    *******************************BEGIN LOW QUALIY PASS*****************************************
    ****************************************************************************************/

    {

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, (GLsizei)win_x_, (GLsizei)win_y_);

        //glPushAttrib(GL_ALL_ATTRIB_BITS);
        glEnable(GL_POINT_SPRITE);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        glEnable(GL_DEPTH_TEST);

        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glDisable(GL_CULL_FACE);

        LQ_one_pass_program_->enable();

        glBindBuffer(GL_ARRAY_BUFFER, render_AB->get_buffer());
        //render_VA->declare_attributes(0);

        size_t size_of_surfel = sizeof(lamure::lod::dataset::serialized_surfel);
        //declare attributes
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, size_of_surfel, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, size_of_surfel, (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, size_of_surfel, (void*)(4*sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, size_of_surfel, (void*)(5*sizeof(float)));


        node_t node_counter = 0;
        node_t non_culled_node_idx = 0;
        for (auto& model_id : current_set) {
            cut& cut = cuts->get_cut(context_id, view_id, model_id);

            std::vector<cut::node_slot_aggregate> renderable = cut.complete_set();

            const bvh* bvh = database->get_model(model_id)->get_bvh();

            if (bvh->get_primitive() != bvh::primitive_type::POINTCLOUD) {
                continue;
            }

            size_t surfels_per_node_of_model = bvh->get_primitives_per_node();
            //store culling result and push it back for second pass#
            std::vector<lamure::math::bounding_box_t>const & bounding_box_vector = bvh->get_bounding_boxes();


            upload_transformation_matrices(camera, model_id, RenderPass::ONE_PASS_LQ);

            lamure::util::frustum_t frustum_by_model = camera.get_frustum_by_model(model_transformations_[model_id]);


            for(auto const& node_slot_aggregate : renderable) {
                uint32_t node_culling_result = camera.cull_against_frustum( frustum_by_model ,bounding_box_vector[ node_slot_aggregate.node_id_ ] );


                frustum_culling_results[node_counter] = node_culling_result;

                if( (node_culling_result != 1) ) {

                    glDrawArrays(GL_POINTS, (GLsizei)(node_slot_aggregate.slot_id_) * (GLsizei)number_of_surfels_per_node, (GLsizei)surfels_per_node_of_model);

                    ++non_culled_node_idx;
                }

                ++node_counter;
            }
       }

       rendered_splats_ = non_culled_node_idx * database->get_primitives_per_node();
    }


}

void Renderer::
render(lamure::context_t context_id, lamure::lod::camera const& camera, const lamure::view_t view_id,
lamure::gl::vertex_array_t* render_VA,
lamure::gl::array_buffer_t* render_AB,
const unsigned current_camera_session)
{
    using namespace lamure;
    using namespace lamure::lod;

    update_frustum_dependent_parameters(camera);
    upload_uniforms(camera);

    using namespace lamure::gl;
    using namespace lamure::math;

    model_database* database = model_database::get_instance();
    cut_database* cuts = cut_database::get_instance();

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
        cut& cut = cuts->get_cut(context_id, view_id, model_id);

        std::vector<cut::node_slot_aggregate> renderable = cut.complete_set();

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

     render_one_pass_LQ(context_id,
                       camera,
                       view_id,
                       render_VA, render_AB,
                       current_set,
                       frustum_culling_results);


}


void Renderer::reset_viewport(int w, int h)
{
    //reset viewport
    win_x_ = w;
    win_y_ = h;
    glViewport(0, 0, w, h);

}


void Renderer::send_model_transform(const lamure::model_t model_id, const lamure::mat4r_t& transform) {
    model_transformations_[model_id] = transform;
}

void Renderer::display_status(std::string const& information_to_display)
{

}

void Renderer::
initialize_VBOs()
{

}

bool Renderer::
initialize_schism_device_and_shaders(int resX, int resY)
{
    std::string shader_path = LAMURE_SHADERS_DIR;

    LQ_one_pass_program_ = new lamure::gl::shader_t();
    LQ_one_pass_program_->attach(GL_VERTEX_SHADER, shader_path + "/lq_one_pass.glslv");
    LQ_one_pass_program_->attach(GL_GEOMETRY_SHADER, shader_path + "/lq_one_pass.glslg");
    LQ_one_pass_program_->attach(GL_FRAGMENT_SHADER, shader_path + "/lq_one_pass.glslf");
    LQ_one_pass_program_->link();


    bounding_box_vis_shader_program_ = new lamure::gl::shader_t();
    bounding_box_vis_shader_program_->attach(GL_VERTEX_SHADER, shader_path + "/bounding_box_vis.glslv");
    bounding_box_vis_shader_program_->attach(GL_FRAGMENT_SHADER, shader_path + "/bounding_box_vis.glslf");
    bounding_box_vis_shader_program_->link();


}

void Renderer::
update_frustum_dependent_parameters(lamure::lod::camera const& camera)
{
    near_plane_ = camera.near_plane_value();
    far_plane_  = camera.far_plane_value();

    std::vector<lamure::math::vec3d_t> corner_values = camera.get_frustum_corners();
    double top_minus_bottom = lamure::math::length((corner_values[2]) - (corner_values[0]));

    height_divided_by_top_minus_bottom_ = win_y_ / top_minus_bottom;
}

void Renderer::
calculate_radius_scale_per_model()
{
    using namespace lamure::lod;
    uint32_t num_models = (model_database::get_instance())->num_models();

    if(radius_scale_per_model_.size() < num_models)
      radius_scale_per_model_.resize(num_models);

    lamure::math::vec4d_t x_unit_vec = lamure::math::vec4d_t(1.0,0.0,0.0,0.0);
    for(unsigned int model_id = 0; model_id < num_models; ++model_id)
    {
      radius_scale_per_model_[model_id] = lamure::math::length(model_transformations_[model_id] * x_unit_vec);
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

}

void Renderer::
switch_render_mode(RenderMode const& render_mode) {
    render_mode_ = render_mode;
}
