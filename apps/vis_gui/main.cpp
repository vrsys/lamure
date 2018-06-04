// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

//gl
#include "imgui_impl_glfw_gl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

//schism
#include <scm/core.h>
#include <scm/core/math.h>
#include <scm/core/io/tools.h>
#include <scm/core/pointer_types.h>
#include <scm/gl_core/gl_core_fwd.h>
#include <scm/gl_util/primitives/primitives_fwd.h>
#include <scm/core/platform/platform.h>
#include <scm/core/utilities/platform_warning_disable.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/font/font_face.h>
#include <scm/gl_util/font/text.h>
#include <scm/gl_util/font/text_renderer.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>
#include <scm/gl_util/data/imaging/texture_loader.h>

//boost
#include <boost/assign/list_of.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

//lamure
#include <lamure/types.h>
#include <lamure/ren/config.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/dataset.h>
#include <lamure/ren/policy.h>
#include <lamure/ren/controller.h>
#include <lamure/pvs/pvs_database.h>
#include <lamure/ren/ray.h>
#include <lamure/prov/aux.h>
#include <lamure/prov/octree.h>


#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <list>

#include "utils.h"
#include "input.h"
#include "selection.h"
#include "provenance.h"
#include "settings.h"
#include "vertex.h"
#include "window.h"
#include "gui.h"

#include "vt_system.h"
#include "shader_manager.h"

using namespace vis_utils;

static const std::string VIS_QUAD_SHADER               = "vis_quad_shader";
static const std::string VIS_XYZ_SHADER                = "vis_xyz_shader";
static const std::string VIS_LINE_SHADER               = "vis_line_shader";
static const std::string VIS_TRIANGLE_SHADER           = "vis_triangle_shader";
static const std::string VIS_XYZ_PASS1_SHADER          = "vis_xyz_pass1_shader";
static const std::string VIS_XYZ_PASS2_SHADER          = "vis_xyz_pass2_shader";
static const std::string VIS_XYZ_PASS3_SHADER          = "vis_xyz_pass3_shader";
static const std::string VIS_XYZ_LIGHTING_SHADER       = "vis_xyz_lighting_shader";
static const std::string VIS_XYZ_PASS2_LIGHTING_SHADER = "vis_xyz_pass2_lighting_shader";
static const std::string VIS_XYZ_PASS3_LIGHTING_SHADER = "vis_xyz_pass3_lighting_shader";
static const std::string VIS_VT_SHADER                 = "vis_vt_shader";

bool rendering_ = false;
int32_t render_width_ = 1280;
int32_t render_height_ = 720;

int32_t num_models_ = 0;
std::vector<scm::math::mat4d> model_transformations_;

float height_divided_by_top_minus_bottom_ = 0.f;

lamure::ren::camera* camera_ = nullptr;

scm::shared_ptr<scm::gl::render_device> device_;
scm::shared_ptr<scm::gl::render_context> context_;

std::shared_ptr<shader_manager> shader_manager_;


scm::gl::frame_buffer_ptr fbo_;
scm::gl::texture_2d_ptr fbo_color_buffer_;
scm::gl::texture_2d_ptr fbo_depth_buffer_;

scm::gl::frame_buffer_ptr pass1_fbo_;
scm::gl::texture_2d_ptr pass1_depth_buffer_;
scm::gl::frame_buffer_ptr pass2_fbo_;
scm::gl::texture_2d_ptr pass2_color_buffer_;
scm::gl::texture_2d_ptr pass2_normal_buffer_;
scm::gl::texture_2d_ptr pass2_view_space_pos_buffer_;
scm::gl::texture_2d_ptr pass2_depth_buffer_;

scm::gl::depth_stencil_state_ptr depth_state_disable_;
scm::gl::depth_stencil_state_ptr depth_state_less_;
scm::gl::depth_stencil_state_ptr depth_state_without_writing_;
scm::gl::rasterizer_state_ptr no_backface_culling_rasterizer_state_;

scm::gl::blend_state_ptr color_blending_state_;
scm::gl::blend_state_ptr color_no_blending_state_;

scm::gl::sampler_state_ptr filter_linear_;
scm::gl::sampler_state_ptr filter_nearest_;

scm::gl::texture_2d_ptr bg_texture_;

resource brush_resource_;
std::map<uint32_t, resource> sparse_resources_;
std::map<uint32_t, resource> frusta_resources_;
std::map<uint32_t, resource> octree_resources_;
std::map<uint32_t, resource> image_plane_resources_;

scm::shared_ptr<scm::gl::quad_geometry> screen_quad_;
scm::time::accum_timer<scm::time::high_res_timer> frame_time_;

double fps_ = 0.0;
uint64_t rendered_splats_ = 0;
uint64_t rendered_nodes_ = 0;

lamure::ren::Data_Provenance data_provenance_;

input input_;

std::shared_ptr<gui> gui_{};

selection selection_;

std::map<uint32_t, provenance> provenance_;

settings settings_;

vt_system *vt_system_ = nullptr;


void set_uniforms(scm::gl::program_ptr shader) {
  shader->uniform("win_size", scm::math::vec2f(render_width_, render_height_));

  shader->uniform("near_plane", settings_.near_plane_);
  shader->uniform("far_plane", settings_.far_plane_);
  shader->uniform("point_size_factor", settings_.lod_point_scale_);

  shader->uniform("show_normals", (bool)settings_.show_normals_);
  shader->uniform("show_accuracy", (bool)settings_.show_accuracy_);
  shader->uniform("show_radius_deviation", (bool)settings_.show_radius_deviation_);
  shader->uniform("show_output_sensitivity", (bool)settings_.show_output_sensitivity_);

  shader->uniform("channel", settings_.channel_);
  shader->uniform("heatmap", (bool)settings_.heatmap_);

  shader->uniform("face_eye", false);

  shader->uniform("heatmap_min", settings_.heatmap_min_);
  shader->uniform("heatmap_max", settings_.heatmap_max_);
  shader->uniform("heatmap_min_color", settings_.heatmap_color_min_);
  shader->uniform("heatmap_max_color", settings_.heatmap_color_max_);

  if (settings_.enable_lighting_) {
    shader->uniform("use_material_color", settings_.use_material_color_);
    shader->uniform("material_diffuse", settings_.material_diffuse_);
    shader->uniform("material_specular", settings_.material_specular_);

    shader->uniform("ambient_light_color", settings_.ambient_light_color_);
    shader->uniform("point_light_color", settings_.point_light_color_);
  }
}



void draw_brush(scm::gl::program_ptr shader) {
  if (brush_resource_.num_primitives_ > 0) {

    set_uniforms(shader);

    scm::math::mat4d model_matrix = scm::math::mat4d::identity();
    scm::math::mat4d projection_matrix = scm::math::mat4d(camera_->get_projection_matrix());
    scm::math::mat4d view_matrix = camera_->get_high_precision_view_matrix();
    scm::math::mat4d model_view_matrix = view_matrix * model_matrix;
    scm::math::mat4d model_view_projection_matrix = projection_matrix * model_view_matrix;

    shader->uniform("mvp_matrix", scm::math::mat4f(model_view_projection_matrix));
    shader->uniform("model_matrix", scm::math::mat4f(model_matrix));
    shader->uniform("model_view_matrix", scm::math::mat4f(model_view_matrix));
    shader->uniform("inv_mv_matrix", scm::math::mat4f(scm::math::transpose(scm::math::inverse(model_view_matrix))));

    shader->uniform("point_size_factor", settings_.aux_point_scale_);

    shader->uniform("model_to_screen_matrix", scm::math::mat4f::identity());
    shader->uniform("model_radius_scale", 1.f);

    shader->uniform("show_normals", false);
    shader->uniform("show_accuracy", false);
    shader->uniform("show_radius_deviation", false);
    shader->uniform("show_output_sensitivity", false);
    shader->uniform("channel", 0);

    shader->uniform("face_eye", false);

    context_->bind_vertex_array(brush_resource_.array_);
    context_->apply();
    context_->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, brush_resource_.num_primitives_);
  }

}


void draw_resources() {

  if (sparse_resources_.size() > 0) { 
    if ((settings_.show_sparse_ || settings_.show_views_) && sparse_resources_.size() > 0) {

      auto vis_xyz_shader_ = shader_manager_->get_program(VIS_XYZ_SHADER);

      context_->bind_program(vis_xyz_shader_);
      context_->set_blend_state(color_no_blending_state_);
      context_->set_depth_stencil_state(depth_state_less_);

      set_uniforms(vis_xyz_shader_);

      scm::math::mat4d model_matrix = scm::math::mat4d::identity();
      scm::math::mat4d projection_matrix = scm::math::mat4d(camera_->get_projection_matrix());
      scm::math::mat4d view_matrix = camera_->get_high_precision_view_matrix();
      scm::math::mat4d model_view_matrix = view_matrix * model_matrix;
      scm::math::mat4d model_view_projection_matrix = projection_matrix * model_view_matrix;

      vis_xyz_shader_->uniform("mvp_matrix", scm::math::mat4f(model_view_projection_matrix));
      vis_xyz_shader_->uniform("model_matrix", scm::math::mat4f(model_matrix));
      vis_xyz_shader_->uniform("model_view_matrix", scm::math::mat4f(model_view_matrix));
      vis_xyz_shader_->uniform("inv_mv_matrix", scm::math::mat4f(scm::math::transpose(scm::math::inverse(model_view_matrix))));

      vis_xyz_shader_->uniform("point_size_factor", settings_.aux_point_scale_);

      vis_xyz_shader_->uniform("model_to_screen_matrix", scm::math::mat4f::identity());
      vis_xyz_shader_->uniform("model_radius_scale", 1.f);

      scm::math::mat4f inv_view = scm::math::inverse(scm::math::mat4f(view_matrix));
      scm::math::vec3f eye = scm::math::vec3f(inv_view[12], inv_view[13], inv_view[14]);

      vis_xyz_shader_->uniform("eye", eye);
      vis_xyz_shader_->uniform("face_eye", true);
      
      vis_xyz_shader_->uniform("show_normals", false);
      vis_xyz_shader_->uniform("show_accuracy", false);
      vis_xyz_shader_->uniform("show_radius_deviation", false);
      vis_xyz_shader_->uniform("show_output_sensitivity", false);
      vis_xyz_shader_->uniform("channel", 0);

      for (int32_t model_id = 0; model_id < num_models_; ++model_id) {
        if (selection_.selected_model_ != -1) {
          model_id = selection_.selected_model_;
        }
        
        auto s_res = sparse_resources_[model_id];
        if (s_res.num_primitives_ > 0) {
          context_->bind_vertex_array(s_res.array_);
          context_->apply();

          uint32_t num_views = provenance_[model_id].num_views_;

          if (settings_.show_views_) {
            if (selection_.selected_views_.empty()) {
              context_->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, num_views);
            }
            else {
              for (const auto view : selection_.selected_views_) {
                context_->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, view, 1);
              }
            }
          }
          if (settings_.show_sparse_) {
            context_->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, num_views, 
              s_res.num_primitives_-num_views);
          }
        
        }

        if (selection_.selected_model_ != -1) {
          break;
        }
      }

    }

    // draw image_plane resources with vt system
    if (settings_.show_photos_ && !settings_.atlas_file_.empty()) {
        vt_system_->render(camera_, render_width_, render_height_, num_models_, selection_);
    }

    if (settings_.show_views_ || settings_.show_octrees_) {
      auto vis_line_shader_ = shader_manager_->get_program(VIS_LINE_SHADER);

      context_->bind_program(vis_line_shader_);

      scm::math::mat4f projection_matrix = scm::math::mat4f(camera_->get_projection_matrix());
      scm::math::mat4f view_matrix = camera_->get_view_matrix();   
      vis_line_shader_->uniform("view_matrix", view_matrix);
      vis_line_shader_->uniform("projection_matrix", projection_matrix);
      
      for (int32_t model_id = 0; model_id < num_models_; ++model_id) {
        if (selection_.selected_model_ != -1) {
          model_id = selection_.selected_model_;
        }
        
        if (settings_.show_views_) {
          uint32_t num_views = provenance_[model_id].num_views_;
          auto f_res = frusta_resources_[model_id];
          if (f_res.num_primitives_ > 0) {
            context_->bind_vertex_array(f_res.array_);
            context_->apply();
            if (selection_.selected_views_.empty()) {
              context_->draw_arrays(scm::gl::PRIMITIVE_LINE_LIST, 0, f_res.num_primitives_);
            }
            else {
              for (const auto view : selection_.selected_views_) {
                context_->draw_arrays(scm::gl::PRIMITIVE_LINE_LIST, view * 16, 16);
              }
            }
          }
        }

        if (settings_.show_octrees_) {
          auto o_res = octree_resources_[model_id];
          if (o_res.num_primitives_ > 0) {
            context_->bind_vertex_array(o_res.array_);
            context_->apply();
            context_->draw_arrays(scm::gl::PRIMITIVE_LINE_LIST, 0, o_res.num_primitives_);
          }
        }

        if (selection_.selected_model_ != -1) {
          break;
        }
      }
    }

    
  }

}

void draw_all_models(const lamure::context_t context_id, const lamure::view_t view_id, scm::gl::program_ptr shader) {

  lamure::ren::controller* controller = lamure::ren::controller::get_instance();
  lamure::ren::cut_database* cuts = lamure::ren::cut_database::get_instance();
  lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
  lamure::pvs::pvs_database* pvs = lamure::pvs::pvs_database::get_instance();

  if (lamure::ren::policy::get_instance()->size_of_provenance() > 0) {
    context_->bind_vertex_array(
      controller->get_context_memory(context_id, lamure::ren::bvh::primitive_type::POINTCLOUD, device_, data_provenance_));
  }
  else {
   context_->bind_vertex_array(
      controller->get_context_memory(context_id, lamure::ren::bvh::primitive_type::POINTCLOUD, device_)); 
  }
  context_->apply();

  rendered_splats_ = 0;
  rendered_nodes_ = 0;

  for (int32_t model_id = 0; model_id < num_models_; ++model_id) {
    if (selection_.selected_model_ != -1) {
      model_id = selection_.selected_model_;
    }
    bool draw = true;
    if (settings_.show_sparse_ && sparse_resources_[model_id].num_primitives_ > 0) {
      if (selection_.selected_model_ != -1) break;
      //else continue; //don't show lod when sparse is already shown
      else draw = false;
    }
    lamure::model_t m_id = controller->deduce_model_id(std::to_string(model_id));
    lamure::ren::cut& cut = cuts->get_cut(context_id, view_id, m_id);
    std::vector<lamure::ren::cut::node_slot_aggregate> renderable = cut.complete_set();
    const lamure::ren::bvh* bvh = database->get_model(m_id)->get_bvh();
    if (bvh->get_primitive() != lamure::ren::bvh::primitive_type::POINTCLOUD) {
      if (selection_.selected_model_ != -1) break;
      //else continue;
      else draw = false;
    }
    
    //uniforms per model
    scm::math::mat4d model_matrix = model_transformations_[model_id];
    scm::math::mat4d projection_matrix = scm::math::mat4d(camera_->get_projection_matrix());
    scm::math::mat4d view_matrix = camera_->get_high_precision_view_matrix();
    scm::math::mat4d model_view_matrix = view_matrix * model_matrix;
    scm::math::mat4d model_view_projection_matrix = projection_matrix * model_view_matrix;

    shader->uniform("mvp_matrix", scm::math::mat4f(model_view_projection_matrix));
    shader->uniform("model_matrix", scm::math::mat4f(model_matrix));
    shader->uniform("model_view_matrix", scm::math::mat4f(model_view_matrix));
    shader->uniform("inv_mv_matrix", scm::math::mat4f(scm::math::transpose(scm::math::inverse(model_view_matrix))));

    const scm::math::mat4f viewport_scale = scm::math::make_scale(render_width_ * 0.5f, render_width_ * 0.5f, 0.5f);;
    const scm::math::mat4f viewport_translate = scm::math::make_translation(1.0f,1.0f,1.0f);
    const scm::math::mat4d model_to_screen =  scm::math::mat4d(viewport_scale) * scm::math::mat4d(viewport_translate) * model_view_projection_matrix;
    shader->uniform("model_to_screen_matrix", scm::math::mat4f(model_to_screen));

    scm::math::vec4d x_unit_vec = scm::math::vec4d(1.0,0.0,0.0,0.0);
    float model_radius_scale = scm::math::length(model_matrix * x_unit_vec);
    shader->uniform("model_radius_scale", model_radius_scale);

    size_t surfels_per_node = database->get_primitives_per_node();
    std::vector<scm::gl::boxf>const & bounding_box_vector = bvh->get_bounding_boxes();
    
    scm::gl::frustum frustum_by_model = camera_->get_frustum_by_model(scm::math::mat4f(model_matrix));
    
    for(auto const& node_slot_aggregate : renderable) {
      uint32_t node_culling_result = camera_->cull_against_frustum(
        frustum_by_model,
        bounding_box_vector[node_slot_aggregate.node_id_]);
        
      if (node_culling_result != 1) {

        if (settings_.use_pvs_ && pvs->is_activated() && settings_.pvs_culling_ 
          && !lamure::pvs::pvs_database::get_instance()->get_viewer_visibility(model_id, node_slot_aggregate.node_id_)) {
          continue;
        }
        
        if (settings_.show_accuracy_) {
          const float accuracy = 1.0 - (bvh->get_depth_of_node(node_slot_aggregate.node_id_) * 1.0)/(bvh->get_depth() - 1);
          shader->uniform("accuracy", accuracy);
        }
        if (settings_.show_radius_deviation_) {
          shader->uniform("average_radius", bvh->get_avg_primitive_extent(node_slot_aggregate.node_id_));
        }

        context_->apply();

        if (draw) {
          context_->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST,
            (node_slot_aggregate.slot_id_) * (GLsizei)surfels_per_node, surfels_per_node);
          rendered_splats_ += surfels_per_node;
          ++rendered_nodes_;
        }

        
      
      }
    }
    if (selection_.selected_model_ != -1) {
      break;
    }
  }


}

void create_brush_resource() {

  if (selection_.brush_.empty()) {
    brush_resource_.num_primitives_ = 0;
    return;
  }
  if (brush_resource_.num_primitives_ == selection_.brush_.size()) {
    return;
  }

  brush_resource_.num_primitives_ = selection_.brush_.size();
  
  brush_resource_.buffer_.reset();
  brush_resource_.array_.reset(); 

  brush_resource_.buffer_ = device_->create_buffer(
    scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, sizeof(xyz) * brush_resource_.num_primitives_, &selection_.brush_[0]);
  brush_resource_.array_ = device_->create_vertex_array(scm::gl::vertex_format
    (0, 0, scm::gl::TYPE_VEC3F, sizeof(xyz))
    (0, 1, scm::gl::TYPE_UBYTE, sizeof(xyz), scm::gl::INT_FLOAT_NORMALIZE)
    (0, 2, scm::gl::TYPE_UBYTE, sizeof(xyz), scm::gl::INT_FLOAT_NORMALIZE)
    (0, 3, scm::gl::TYPE_UBYTE, sizeof(xyz), scm::gl::INT_FLOAT_NORMALIZE)
    (0, 4, scm::gl::TYPE_UBYTE, sizeof(xyz), scm::gl::INT_FLOAT_NORMALIZE)
    (0, 5, scm::gl::TYPE_FLOAT, sizeof(xyz))
    (0, 6, scm::gl::TYPE_VEC3F, sizeof(xyz)),
    boost::assign::list_of(brush_resource_.buffer_));

}

void glut_display() {
  if (rendering_) {
    return;
  }
  rendering_ = true;

  camera_->set_projection_matrix(settings_.fov_, float(settings_.width_)/float(settings_.height_),  settings_.near_plane_, settings_.far_plane_);


  lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
  lamure::ren::cut_database* cuts = lamure::ren::cut_database::get_instance();
  lamure::ren::controller* controller = lamure::ren::controller::get_instance();
  lamure::pvs::pvs_database* pvs = lamure::pvs::pvs_database::get_instance();

  if (lamure::ren::policy::get_instance()->size_of_provenance() > 0) {
    controller->reset_system(data_provenance_);
  }
  else {
    controller->reset_system();
  }
  lamure::context_t context_id = controller->deduce_context_id(0);
  
  for (lamure::model_t model_id = 0; model_id < num_models_; ++model_id) {
    lamure::model_t m_id = controller->deduce_model_id(std::to_string(model_id));

    cuts->send_transform(context_id, m_id, scm::math::mat4f(model_transformations_[m_id]));
    cuts->send_threshold(context_id, m_id, settings_.lod_error_);
    cuts->send_rendered(context_id, m_id);
    
    database->get_model(m_id)->set_transform(scm::math::mat4f(model_transformations_[m_id]));
  }


  lamure::view_t cam_id = controller->deduce_view_id(context_id, camera_->view_id());
  cuts->send_camera(context_id, cam_id, *camera_);

  std::vector<scm::math::vec3d> corner_values = camera_->get_frustum_corners();
  double top_minus_bottom = scm::math::length((corner_values[2]) - (corner_values[0]));
  height_divided_by_top_minus_bottom_ = lamure::ren::policy::get_instance()->window_height() / top_minus_bottom;

  cuts->send_height_divided_by_top_minus_bottom(context_id, cam_id, height_divided_by_top_minus_bottom_);

  if (settings_.use_pvs_) {
    scm::math::mat4f cm = scm::math::inverse(scm::math::mat4f(camera_->trackball_matrix()));
    scm::math::vec3d cam_pos = scm::math::vec3d(cm[12], cm[13], cm[14]);
    pvs->set_viewer_position(cam_pos);
  }

  if (settings_.lod_update_) {
    if (lamure::ren::policy::get_instance()->size_of_provenance() > 0) {
      controller->dispatch(context_id, device_, data_provenance_);
    }
    else {
      controller->dispatch(context_id, device_); 
    }
  }
  lamure::view_t view_id = controller->deduce_view_id(context_id, camera_->view_id());
 
  
  create_brush_resource();

  context_->set_rasterizer_state(no_backface_culling_rasterizer_state_);

  if (settings_.splatting_) {
    //2 pass splatting
    //PASS 1

    context_->clear_color_buffer(pass1_fbo_ , 0, scm::math::vec4f( .0f, .0f, .0f, 0.0f));
    context_->clear_depth_stencil_buffer(pass1_fbo_);
    context_->set_frame_buffer(pass1_fbo_);

    auto vis_xyz_pass1_shader_ = shader_manager_->get_program(VIS_XYZ_PASS1_SHADER);
      
    context_->bind_program(vis_xyz_pass1_shader_);
    context_->set_blend_state(color_no_blending_state_);
    context_->set_depth_stencil_state(depth_state_less_);

    set_uniforms(vis_xyz_pass1_shader_);

    context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(render_width_, render_height_)));
    context_->apply();

    draw_all_models(context_id, view_id, vis_xyz_pass1_shader_);

    draw_brush(vis_xyz_pass1_shader_);

    //PASS 2

    context_->clear_color_buffer(pass2_fbo_ , 0, scm::math::vec4f( .0f, .0f, .0f, 0.0f));
    context_->clear_color_buffer(pass2_fbo_ , 1, scm::math::vec4f( .0f, .0f, .0f, 0.0f));
    context_->clear_color_buffer(pass2_fbo_ , 2, scm::math::vec4f( .0f, .0f, .0f, 0.0f));
    
    context_->set_frame_buffer(pass2_fbo_);

    context_->set_blend_state(color_blending_state_);
    context_->set_depth_stencil_state(depth_state_without_writing_);
    context_->set_rasterizer_state(no_backface_culling_rasterizer_state_);

    auto selected_pass2_shading_program = shader_manager_->get_program(VIS_XYZ_PASS2_SHADER);

    if(settings_.enable_lighting_) {
      selected_pass2_shading_program = shader_manager_->get_program(VIS_XYZ_PASS2_LIGHTING_SHADER);
    }

    context_->bind_program(selected_pass2_shading_program);

    set_uniforms(selected_pass2_shading_program);

    context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(render_width_, render_height_)));
    context_->apply();

    draw_all_models(context_id, view_id, selected_pass2_shading_program);

    draw_brush(selected_pass2_shading_program);

    //PASS 3

    context_->clear_color_buffer(fbo_, 0, scm::math::vec4f(0.0, 0.0, 0.0, 1.0f));
    context_->clear_depth_stencil_buffer(fbo_);
    context_->set_frame_buffer(fbo_);
    
    context_->set_depth_stencil_state(depth_state_disable_);

    auto selected_pass3_shading_program = shader_manager_->get_program(VIS_XYZ_PASS3_SHADER);

    if(settings_.enable_lighting_) {
      selected_pass3_shading_program = shader_manager_->get_program(VIS_XYZ_PASS3_LIGHTING_SHADER);
    }

    context_->bind_program(selected_pass3_shading_program);

    set_uniforms(selected_pass3_shading_program);

    selected_pass3_shading_program->uniform("background_color", 
      scm::math::vec3f(settings_.background_color_.x, settings_.background_color_.y, settings_.background_color_.z));

    selected_pass3_shading_program->uniform_sampler("in_color_texture", 0);
    context_->bind_texture(pass2_color_buffer_, filter_nearest_, 0);

    if(settings_.enable_lighting_) {
      context_->bind_texture(pass2_normal_buffer_, filter_nearest_, 1);
      context_->bind_texture(pass2_view_space_pos_buffer_, filter_nearest_, 2);
    }

    context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(render_width_, render_height_)));
    context_->apply();

    screen_quad_->draw(context_);
    
  }
  else {
    //single pass

    context_->clear_color_buffer(fbo_, 0,
      scm::math::vec4f(settings_.background_color_.x, settings_.background_color_.y, settings_.background_color_.z, 1.0f));
    context_->clear_depth_stencil_buffer(fbo_);
    context_->set_frame_buffer(fbo_);

    auto selected_single_pass_shading_program = shader_manager_->get_program(VIS_XYZ_SHADER);

    if(settings_.enable_lighting_) {
      selected_single_pass_shading_program = shader_manager_->get_program(VIS_XYZ_LIGHTING_SHADER);
    }

    context_->bind_program(selected_single_pass_shading_program);
    context_->set_blend_state(color_no_blending_state_);
    context_->set_depth_stencil_state(depth_state_less_);
    
    set_uniforms(selected_single_pass_shading_program);
    /*if (settings_.background_image_ != "") {
      context_->bind_texture(bg_texture_, filter_linear_, 0);
      selected_single_pass_shading_program->uniform("background_image", true);
    }*/

    context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(render_width_, render_height_)));
    context_->apply();

    draw_all_models(context_id, view_id, selected_single_pass_shading_program);

    context_->bind_program(shader_manager_->get_program(VIS_XYZ_SHADER));
    draw_brush(shader_manager_->get_program(VIS_XYZ_SHADER));
    draw_resources();


  }


  //PASS 4: fullscreen quad
  
  context_->clear_default_depth_stencil_buffer();
  context_->clear_default_color_buffer();
  context_->set_default_frame_buffer();
  context_->set_depth_stencil_state(depth_state_disable_);

  auto vis_quad_shader_ = shader_manager_->get_program(VIS_QUAD_SHADER);
  context_->bind_program(vis_quad_shader_);
  
  context_->bind_texture(fbo_color_buffer_, filter_linear_, 0);
  vis_quad_shader_->uniform("gamma_correction", (bool)settings_.gamma_correction_);

  context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(settings_.width_, settings_.height_)));
  context_->apply();
  
  screen_quad_->draw(context_);

  rendering_ = false;
  //glutSwapBuffers();

  frame_time_.stop();
  frame_time_.start();
  //schism bug ? time::to_seconds yields milliseconds
  if (scm::time::to_seconds(frame_time_.accumulated_duration()) > 100.0) {
    fps_ = 1000.0f / scm::time::to_seconds(frame_time_.average_duration());
    frame_time_.reset();
  }
  
}


scm::math::vec3f deproject(float x, float y, float z) {
  scm::math::mat4f matrix_inverse = scm::math::inverse(camera_->get_projection_matrix() * camera_->get_view_matrix());

  float x_normalized = 2.0f * x / settings_.width_ - 1;
  float y_normalized = -2.0f * y / settings_.height_ + 1;

  scm::math::vec4f point_screen = scm::math::vec4f(x_normalized, y_normalized, z, 1.0f);
  scm::math::vec4f point_world = matrix_inverse * point_screen;

  return scm::math::vec3f(point_world[0] / point_world[3], point_world[1] / point_world[3], point_world[2] / point_world[3]);
}



void brush() {
  
  if (!input_.brush_mode_) {
    return;
  }

  if (input_.mouse_state_.rb_down_) {
    selection_.brush_.clear();
    selection_.selected_views_.clear();
    return;
  }

  if (!input_.mouse_state_.lb_down_) {
    return;
  }

  if (scm::math::length(input_.mouse_-input_.prev_mouse_) < 2) {
    return;
  }

  lamure::ren::model_database *database = lamure::ren::model_database::get_instance();

  scm::math::vec3f front = deproject(input_.mouse_.x, input_.mouse_.y, -1.0);
  scm::math::vec3f back = deproject(input_.mouse_.x, input_.mouse_.y, 1.0);
  scm::math::vec3f direction_ray = back - front;

  lamure::ren::ray ray_brush(front, direction_ray, 100000.0f);

  bool hit = false;

  uint32_t max_depth = 255;
  uint32_t surfel_skip = 1;
  lamure::ren::ray::intersection intersection;
  if (selection_.selected_model_ != -1) {
    scm::math::mat4f model_transform = database->get_model(selection_.selected_model_)->transform();
    if (ray_brush.intersect_model(selection_.selected_model_, model_transform, 1.0f, max_depth, surfel_skip, false, intersection)) {
      hit = true;
    }
  }
  else {
    scm::math::mat4f cm = scm::math::inverse(scm::math::mat4f(camera_->trackball_matrix()));
    scm::math::vec3f cam_up = scm::math::normalize(scm::math::vec3f(cm[0], cm[1], cm[2]));
    float plane_dim = 0.1f;
    if (ray_brush.intersect(1.0f, cam_up, plane_dim, max_depth, surfel_skip, intersection)) {
      hit = true;
    }
  }

  if (scm::math::length(intersection.position_) < 0.000001f) {
    return;
  }

  if (hit) {
    auto color = scm::math::vec3f(255.f, 240.f, 0) * 0.9f + 0.1f * (scm::math::vec3f(intersection.normal_*0.5f+0.5f)*255);
    selection_.brush_.push_back(
      xyz{
        intersection.position_ + intersection.normal_ * settings_.aux_point_size_,
        (uint8_t)color.x, (uint8_t)color.y, (uint8_t)color.z, (uint8_t)255,
        settings_.aux_point_size_,
        intersection.normal_});

    if (selection_.selected_model_ != -1) {
      if (settings_.octrees_.size() > selection_.selected_model_) {
        if (settings_.octrees_[selection_.selected_model_]) {
          uint64_t selected_node_id = settings_.octrees_[selection_.selected_model_]->query(intersection.position_);
          if (selected_node_id > 0) {
            const std::set<uint32_t>& imgs = settings_.octrees_[selection_.selected_model_]->get_node(selected_node_id).get_fotos();
            std::cout << "found " << imgs.size() << " of " << provenance_[selection_.selected_model_].num_views_ << " imgs" << std::endl;
            //std::cout << "selected_node_id " << selected_node_id << std::endl;
            selection_.selected_views_.insert(imgs.begin(), imgs.end());
          }
        }
      }
    }


  }

}

void create_framebuffers() {

  fbo_.reset();
  fbo_color_buffer_.reset();
  fbo_depth_buffer_.reset();
  pass1_fbo_.reset();
  pass1_depth_buffer_.reset();
  pass2_fbo_.reset();
  pass2_color_buffer_.reset();
  pass2_normal_buffer_.reset();
  pass2_view_space_pos_buffer_.reset();

  fbo_ = device_->create_frame_buffer();
  fbo_color_buffer_ = device_->create_texture_2d(scm::math::vec2ui(render_width_, render_height_), scm::gl::FORMAT_RGBA_32F , 1, 1, 1);
  fbo_depth_buffer_ = device_->create_texture_2d(scm::math::vec2ui(render_width_, render_height_), scm::gl::FORMAT_D24, 1, 1, 1);
  fbo_->attach_color_buffer(0, fbo_color_buffer_);
  fbo_->attach_depth_stencil_buffer(fbo_depth_buffer_);

  pass1_fbo_ = device_->create_frame_buffer();
  pass1_depth_buffer_ = device_->create_texture_2d(scm::math::vec2ui(render_width_, render_height_), scm::gl::FORMAT_D24, 1, 1, 1);
  pass1_fbo_->attach_depth_stencil_buffer(pass1_depth_buffer_);

  pass2_fbo_ = device_->create_frame_buffer();
  pass2_color_buffer_ = device_->create_texture_2d(scm::math::vec2ui(render_width_, render_height_), scm::gl::FORMAT_RGBA_32F, 1, 1, 1);
  pass2_fbo_->attach_color_buffer(0, pass2_color_buffer_);
  pass2_fbo_->attach_depth_stencil_buffer(pass1_depth_buffer_);

  pass2_normal_buffer_ = device_->create_texture_2d(scm::math::vec2ui(render_width_, render_height_), scm::gl::FORMAT_RGB_32F, 1, 1, 1);
  pass2_fbo_->attach_color_buffer(1, pass2_normal_buffer_);
  pass2_view_space_pos_buffer_ = device_->create_texture_2d(scm::math::vec2ui(render_width_, render_height_), scm::gl::FORMAT_RGB_32F, 1, 1, 1);
  pass2_fbo_->attach_color_buffer(2, pass2_view_space_pos_buffer_);

}



void glut_resize(int32_t w, int32_t h) {
  settings_.width_ = w;
  settings_.height_ = h;
/*
  render_width_ = settings_.width_ / settings_.frame_div_;
  render_height_ = settings_.height_ / settings_.frame_div_;

  create_framebuffers();

  lamure::ren::policy* policy = lamure::ren::policy::get_instance();
  policy->set_window_width(render_width_);
  policy->set_window_height(render_height_);
  
  context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(render_width_, render_height_)));
*/
  camera_->set_projection_matrix(settings_.fov_, float(settings_.width_)/float(settings_.height_),  settings_.near_plane_, settings_.far_plane_);
}


void glut_keyboard(unsigned char key, int32_t x, int32_t y) {

  //std::cout << std::to_string(key) << std::endl;

  int k = (int)key;

  switch (k) {
    case 27:
      exit(0);
      break;

    //case '.':
      //glutFullScreenToggle();
      //break;

    case 'F':
      {
        ++settings_.travel_;
        if(settings_.travel_ > 4){
          settings_.travel_ = 0;
        }
        settings_.travel_speed_ = (settings_.travel_ == 0 ? 0.5f
                  : settings_.travel_ == 1 ? 5.5f
                  : settings_.travel_ == 2 ? 20.5f
                  : settings_.travel_ == 3 ? 100.5f
                  : 300.5f);
        camera_->set_dolly_sens_(settings_.travel_speed_);
      }
      break;
      
    case '0':
      selection_.selected_model_ = -1;
      break;
    case '-':
      if (--selection_.selected_model_ < 0) selection_.selected_model_ = num_models_-1;
      break;
    case '=':
      if (++selection_.selected_model_ >= num_models_) selection_.selected_model_ = 0;
      break;

    case 'Z':
      //dump camera transform
      std::cout << "view_tf: " << std::endl;
      std::cout << camera_->get_high_precision_view_matrix() << std::endl;
      break;

    case ' ':
      settings_.gui_ = !settings_.gui_;
      break;



    default:
      break;

  }

}


void glut_motion(int32_t x, int32_t y) {

  if (input_.gui_lock_) {
    input_.prev_mouse_ = scm::math::vec2i(x, y);
    input_.mouse_ = scm::math::vec2i(x, y);
    return;
  }

  input_.prev_mouse_ = input_.mouse_;
  input_.mouse_ = scm::math::vec2i(x, y);
  
  if (!input_.brush_mode_) {
    camera_->update_trackball(x, y, settings_.width_, settings_.height_, input_.mouse_state_);
  }
  else {
    brush();
  }
}


void create_aux_resources() {

  for (const auto& aux_file : settings_.aux_) {
    if (aux_file.second != "") {
      
      uint32_t model_id = aux_file.first;

      std::cout << "aux: " << aux_file.second << std::endl;
      lamure::prov::aux aux(aux_file.second);

      provenance_[model_id].num_views_ = aux.get_num_views();
      std::cout << "aux: " << aux.get_num_views() << " views" << std::endl;
      std::cout << "aux: " << aux.get_num_sparse_points() << " points" << std::endl;
      std::cout << "aux: " << aux.get_atlas().atlas_width_ << ", " << aux.get_atlas().atlas_height_ << " is it rotated? : " << aux.get_atlas().rotated_ << std::endl;
      std::cout << "aux: " << aux.get_num_atlas_tiles() << " atlas tiles" << std::endl;

      std::vector<xyz> ready_to_upload;

      for (uint32_t i = 0; i < aux.get_num_views(); ++i) {
        const auto& view = aux.get_view(i);
        ready_to_upload.push_back(
          xyz{view.position_,
            (uint8_t)255, (uint8_t)240, (uint8_t)0, (uint8_t)255,
            settings_.aux_point_size_,
            scm::math::vec3f(1.0, 0.0, 0.0)} //placeholder
        );
      }

      for (uint32_t i = 0; i < aux.get_num_sparse_points(); ++i) {
        const auto& point = aux.get_sparse_point(i);
        ready_to_upload.push_back(
          xyz{point.pos_,
            point.r_, point.g_, point.b_, point.a_,
            settings_.aux_point_size_,
            scm::math::vec3f(1.0, 0.0, 0.0)} //placeholder
        );
      }

      resource point_res;
      point_res.num_primitives_ = ready_to_upload.size();
      point_res.buffer_.reset();
      point_res.array_.reset();

      point_res.buffer_ = device_->create_buffer(
        scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, sizeof(xyz) * ready_to_upload.size(), &ready_to_upload[0]);
      point_res.array_ = device_->create_vertex_array(scm::gl::vertex_format
        (0, 0, scm::gl::TYPE_VEC3F, sizeof(xyz))
        (0, 1, scm::gl::TYPE_UBYTE, sizeof(xyz), scm::gl::INT_FLOAT_NORMALIZE)
        (0, 2, scm::gl::TYPE_UBYTE, sizeof(xyz), scm::gl::INT_FLOAT_NORMALIZE)
        (0, 3, scm::gl::TYPE_UBYTE, sizeof(xyz), scm::gl::INT_FLOAT_NORMALIZE)
        (0, 4, scm::gl::TYPE_UBYTE, sizeof(xyz), scm::gl::INT_FLOAT_NORMALIZE)
        (0, 5, scm::gl::TYPE_FLOAT, sizeof(xyz))
        (0, 6, scm::gl::TYPE_VEC3F, sizeof(xyz)),
        boost::assign::list_of(point_res.buffer_));

      sparse_resources_[model_id] = point_res;

      //init octree
      settings_.octrees_[model_id] = aux.get_octree();
      std::cout << "Octree loaded (" << settings_.octrees_[model_id]->get_num_nodes() << " nodes)" << std::endl;
      
      //init octree buffers
      resource octree_res;
      octree_res.buffer_.reset();
      octree_res.array_.reset();

      std::vector<scm::math::vec3f> octree_lines_to_upload;
      for (uint64_t i = 0; i < settings_.octrees_[model_id]->get_num_nodes(); ++i) {
        const auto& node = settings_.octrees_[model_id]->get_node(i);

        const auto min_vertex = scm::math::vec3d(node.get_min().x, node.get_min().y, node.get_min().z);
        const auto max_vertex = scm::math::vec3d(node.get_max().x, node.get_max().y, node.get_max().z);

        octree_lines_to_upload.push_back(scm::math::vec3f(min_vertex.x, min_vertex.y, min_vertex.z));
        octree_lines_to_upload.push_back(scm::math::vec3f(max_vertex.x, min_vertex.y, min_vertex.z));

        octree_lines_to_upload.push_back(scm::math::vec3f(max_vertex.x, min_vertex.y, min_vertex.z));
        octree_lines_to_upload.push_back(scm::math::vec3f(max_vertex.x, min_vertex.y, max_vertex.z));

        octree_lines_to_upload.push_back(scm::math::vec3f(max_vertex.x, min_vertex.y, max_vertex.z));
        octree_lines_to_upload.push_back(scm::math::vec3f(min_vertex.x, min_vertex.y, max_vertex.z));

        octree_lines_to_upload.push_back(scm::math::vec3f(min_vertex.x, min_vertex.y, max_vertex.z));
        octree_lines_to_upload.push_back(scm::math::vec3f(min_vertex.x, min_vertex.y, min_vertex.z));


        octree_lines_to_upload.push_back(scm::math::vec3f(min_vertex.x, max_vertex.y, min_vertex.z));
        octree_lines_to_upload.push_back(scm::math::vec3f(max_vertex.x, max_vertex.y, min_vertex.z));

        octree_lines_to_upload.push_back(scm::math::vec3f(max_vertex.x, max_vertex.y, min_vertex.z));
        octree_lines_to_upload.push_back(scm::math::vec3f(max_vertex.x, max_vertex.y, max_vertex.z));

        octree_lines_to_upload.push_back(scm::math::vec3f(max_vertex.x, max_vertex.y, max_vertex.z));
        octree_lines_to_upload.push_back(scm::math::vec3f(min_vertex.x, max_vertex.y, max_vertex.z));

        octree_lines_to_upload.push_back(scm::math::vec3f(min_vertex.x, max_vertex.y, max_vertex.z));
        octree_lines_to_upload.push_back(scm::math::vec3f(min_vertex.x, max_vertex.y, min_vertex.z));


        octree_lines_to_upload.push_back(scm::math::vec3f(min_vertex.x, min_vertex.y, min_vertex.z));
        octree_lines_to_upload.push_back(scm::math::vec3f(min_vertex.x, max_vertex.y, min_vertex.z));

        octree_lines_to_upload.push_back(scm::math::vec3f(max_vertex.x, min_vertex.y, min_vertex.z));
        octree_lines_to_upload.push_back(scm::math::vec3f(max_vertex.x, max_vertex.y, min_vertex.z));

        octree_lines_to_upload.push_back(scm::math::vec3f(max_vertex.x, min_vertex.y, max_vertex.z));
        octree_lines_to_upload.push_back(scm::math::vec3f(max_vertex.x, max_vertex.y, max_vertex.z));

        octree_lines_to_upload.push_back(scm::math::vec3f(min_vertex.x, min_vertex.y, max_vertex.z));
        octree_lines_to_upload.push_back(scm::math::vec3f(min_vertex.x, max_vertex.y, max_vertex.z));
      }

      octree_res.buffer_ = device_->create_buffer(scm::gl::BIND_VERTEX_BUFFER, 
        scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 3) * octree_lines_to_upload.size(), &octree_lines_to_upload[0]);
      octree_res.array_ = device_->create_vertex_array(scm::gl::vertex_format
        (0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3), 
        boost::assign::list_of(octree_res.buffer_));

      octree_res.num_primitives_ = octree_lines_to_upload.size();
      octree_resources_[model_id] = octree_res;

      auto root_bb = lamure::ren::model_database::get_instance()->get_model(model_id)->get_bvh()->get_bounding_boxes()[0];
      auto root_bb_min = scm::math::mat4f(model_transformations_[model_id]) * root_bb.min_vertex();
      auto root_bb_max = scm::math::mat4f(model_transformations_[model_id]) * root_bb.max_vertex();
      auto model_dim = scm::math::length(root_bb_max - root_bb_min);

      //for image planes
      if (!settings_.atlas_file_.empty()) {
        if (aux.get_num_atlas_tiles() != aux.get_num_views()) {
          throw std::runtime_error(
                  "Number of atlas_tiles (" + std::to_string(aux.get_num_atlas_tiles()) + ") "
                  + "does not match number of views (" + std::to_string(aux.get_num_views()) + ")");
        }

        std::vector<vertex> tris_to_upload;
        for (uint32_t i = 0; i < aux.get_num_views(); ++i) {
          const auto& view       = aux.get_view(i);
          const auto& atlas_tile = aux.get_atlas_tile(i);

          float aspect_ratio = view.image_height_ / (float)view.image_width_;
          float img_w_half   = (settings_.aux_focal_length_)*0.5f;
          float img_h_half   = img_w_half * aspect_ratio;
          float focal_length = settings_.aux_focal_length_;

          float atlas_width  = aux.get_atlas().atlas_width_;
          float atlas_height = aux.get_atlas().atlas_height_;

          // scale factor from image space to vt atlas space
          float factor = vt_system_->get_atlas_scale_factor();

          // positions in vt atlas space coordinate system
          float tile_height  = (float) atlas_tile.width_  / atlas_width  * factor;
          float tile_width   = (float) atlas_tile.width_  / atlas_height * factor;

          float tile_pos_x   = (float) atlas_tile.x_ / atlas_height * factor;
          float tile_pos_y   = (float) atlas_tile.y_ / atlas_tile.height_ * tile_height + (1 - factor);


          vertex p1;
          p1.pos_ = view.transform_ * scm::math::vec3f(-img_w_half, img_h_half, -focal_length);
          p1.uv_  = scm::math::vec2f(tile_pos_x + tile_width, tile_pos_y);

          vertex p2;
          p2.pos_ = view.transform_ * scm::math::vec3f(img_w_half, img_h_half, -focal_length);
          p2.uv_  = scm::math::vec2f(tile_pos_x, tile_pos_y);

          vertex p3;
          p3.pos_ = view.transform_ * scm::math::vec3f(-img_w_half, -img_h_half, -focal_length);
          p3.uv_  = scm::math::vec2f(tile_pos_x + tile_width, tile_pos_y + tile_height);

          vertex p4;
          p4.pos_ = view.transform_ * scm::math::vec3f(img_w_half, -img_h_half, -focal_length);
          p4.uv_  = scm::math::vec2f(tile_pos_x, tile_pos_y + tile_height);

          // left quad triangle
          tris_to_upload.push_back(p1);
          tris_to_upload.push_back(p4);
          tris_to_upload.push_back(p3);

          // right quad triangle
          tris_to_upload.push_back(p2);
          tris_to_upload.push_back(p4);
          tris_to_upload.push_back(p1);
        }

        //init triangle buffer
        resource tri_res;
        tri_res.buffer_.reset();
        tri_res.array_.reset();

        tri_res.buffer_ = device_->create_buffer(scm::gl::BIND_VERTEX_BUFFER,
                                                 scm::gl::USAGE_STATIC_DRAW,
                                                 (sizeof(vertex)) * tris_to_upload.size(),
                                                 &tris_to_upload[0]);

        tri_res.array_ = device_->create_vertex_array(scm::gl::vertex_format
                                                              (0, 0, scm::gl::TYPE_VEC3F, sizeof(vertex))
                                                              (0, 1, scm::gl::TYPE_VEC2F, sizeof(vertex)),
                                                      boost::assign::list_of(tri_res.buffer_));


        tri_res.num_primitives_ = tris_to_upload.size();

        image_plane_resources_[model_id] = tri_res;
      }

      vt_system_->set_image_resources(image_plane_resources_);

      //init line buffers
      resource line_res;
      line_res.buffer_.reset();
      line_res.array_.reset();

      std::vector<scm::math::vec3f> lines_to_upload;
      for (uint32_t i = 0; i < aux.get_num_views(); ++i) {
        const auto& view = aux.get_view(i);

        float aspect_ratio = view.image_height_/(float)view.image_width_;
        float img_w_half = (settings_.aux_focal_length_)*0.5f;
        float img_h_half = img_w_half*aspect_ratio;
        float focal_length = settings_.aux_focal_length_;

        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(-img_w_half, img_h_half, -focal_length));
        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(img_w_half, img_h_half, -focal_length));

        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(img_w_half, img_h_half, -focal_length));
        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(img_w_half, -img_h_half, -focal_length));

        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(img_w_half, -img_h_half, -focal_length));
        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(-img_w_half, -img_h_half, -focal_length));

        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(-img_w_half, -img_h_half, -focal_length));
        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(-img_w_half, img_h_half, -focal_length));

        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(0.f));
        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(-img_w_half, img_h_half, -focal_length));

        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(0.f));
        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(img_w_half, img_h_half, -focal_length));

        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(0.f));
        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(img_w_half, -img_h_half, -focal_length));

        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(0.f));
        lines_to_upload.push_back(view.transform_ * scm::math::vec3f(-img_w_half, -img_h_half, -focal_length));

      }

      line_res.buffer_ = device_->create_buffer(scm::gl::BIND_VERTEX_BUFFER, 
        scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 3) * lines_to_upload.size(), &lines_to_upload[0]);
      line_res.array_ = device_->create_vertex_array(scm::gl::vertex_format
        (0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3), 
        boost::assign::list_of(line_res.buffer_));

      line_res.num_primitives_ = lines_to_upload.size();

      frusta_resources_[model_id] = line_res;

    }
  }

}


std::list<Window *> _windows;
Window *_current_context = nullptr;

class EventHandler {
  public:
    static void on_error(int _err_code, const char *err_msg) { throw std::runtime_error(err_msg); }
    static void on_window_resize(GLFWwindow *glfw_window, int width, int height) {
      Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
      window->_height = (uint32_t)height;
      window->_width = (uint32_t)width;
      glut_resize(width, height);

    }
    static void on_window_key_press(GLFWwindow *glfw_window, int key, int scancode, int action, int mods) {
      if (action == GLFW_RELEASE)
        return;

      Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
      switch(key) {
        case GLFW_KEY_ESCAPE:
          glfwSetWindowShouldClose(glfw_window, GL_TRUE);
          break;
        default:
          glut_keyboard((uint8_t)key, 0, 0);
          break;
      }

      ImGui_ImplGlfwGL3_KeyCallback(glfw_window, key, scancode, action, mods);

    }

    static void on_window_char(GLFWwindow *glfw_window, unsigned int codepoint) {
      Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
      ImGui_ImplGlfwGL3_CharCallback(glfw_window, codepoint);
    }

    static void on_window_button_press(GLFWwindow *glfw_window, int button, int action, int mods) {
      Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
      if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        window->_mouse_button_state = Window::MouseButtonState::LEFT;
      }
      else if(button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
        window->_mouse_button_state = Window::MouseButtonState::WHEEL;
      }
      else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        window->_mouse_button_state = Window::MouseButtonState::RIGHT;
      }
      else {
        window->_mouse_button_state = Window::MouseButtonState::IDLE;
      }

      if (action == GLFW_RELEASE) {
        input_.gui_lock_ = false;
      }
      
      ImGui_ImplGlfwGL3_MouseButtonCallback(glfw_window, button, action, mods);
    }

    static void on_window_move_cursor(GLFWwindow *glfw_window, double x, double y) {
      Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
      
      if (input_.gui_lock_) {
        input_.prev_mouse_ = scm::math::vec2i(x, y);
        input_.mouse_ = scm::math::vec2i(x, y);
        return;
      }

      input_.prev_mouse_ = input_.mouse_;
      input_.mouse_ = scm::math::vec2i(x, y);
  
      if (!input_.brush_mode_) {
        camera_->update_trackball(x, y, settings_.width_, settings_.height_, input_.mouse_state_);
      }
      else {
        brush();
      }

      input_.mouse_state_.lb_down_ = (window->_mouse_button_state == Window::MouseButtonState::LEFT) ? true : false;
      input_.mouse_state_.mb_down_ = (window->_mouse_button_state == Window::MouseButtonState::WHEEL) ? true : false;
      input_.mouse_state_.rb_down_ = (window->_mouse_button_state == Window::MouseButtonState::RIGHT) ? true : false;

      input_.prev_mouse_ = input_.mouse_;
      input_.mouse_ = scm::math::vec2i(x, y);

      if (!input_.brush_mode_) {
        input_.trackball_x_ = 2.f * float(x - (settings_.width_/2))/float(settings_.width_) ;
        input_.trackball_y_ = 2.f * float(settings_.height_ - y - (settings_.height_/2))/float(settings_.height_);
      
        camera_->update_trackball_mouse_pos(input_.trackball_x_, input_.trackball_y_);
      }
      else {
        brush();
      }
    }

    static void on_window_scroll(GLFWwindow *glfw_window, double xoffset, double yoffset) {
      Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);

      ImGui_ImplGlfwGL3_ScrollCallback(glfw_window, xoffset, yoffset);

    }
    
    static void on_window_enter(GLFWwindow *glfw_window, int entered) {
      Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
    }
};

void make_context_current(Window *_window) {
  if(_window != nullptr) {
    glfwMakeContextCurrent(_window->_glfw_window);
    _current_context = _window;
  }
}

Window *create_window(unsigned int width, unsigned int height, const std::string &title, GLFWmonitor *monitor, Window *share) {
  Window *previous_context = _current_context;

  Window *new_window = new Window();

  new_window->_glfw_window = nullptr;
  new_window->_width = width;
  new_window->_height = height;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, 1);
  glfwWindowHint(GLFW_FOCUSED, 1);

  //glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, false);


  if(share != nullptr) {
    new_window->_glfw_window = glfwCreateWindow(width, height, title.c_str(), monitor, share->_glfw_window);
  }
  else {
    new_window->_glfw_window = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
  }

  if(new_window->_glfw_window == nullptr) {
    std::runtime_error("GLFW window creation failed");
  }

  make_context_current(new_window);

  glfwSetKeyCallback(new_window->_glfw_window, &EventHandler::on_window_key_press);
  glfwSetCharCallback(new_window->_glfw_window, &EventHandler::on_window_char);
  glfwSetMouseButtonCallback(new_window->_glfw_window, &EventHandler::on_window_button_press);
  glfwSetCursorPosCallback(new_window->_glfw_window, &EventHandler::on_window_move_cursor);
  glfwSetScrollCallback(new_window->_glfw_window, &EventHandler::on_window_scroll);
  glfwSetCursorEnterCallback(new_window->_glfw_window, &EventHandler::on_window_enter);
  glfwSetWindowSizeCallback(new_window->_glfw_window, &EventHandler::on_window_resize);

  glfwSetWindowUserPointer(new_window->_glfw_window, new_window);

  _windows.push_back(new_window);

  make_context_current(previous_context);

  return new_window;
}

bool should_close() {
  if(_windows.empty())
    return true;

  std::list<Window *> to_delete;
  for(const auto &window : _windows) {
    if(glfwWindowShouldClose(window->_glfw_window)) {
      to_delete.push_back(window);
    }
  }

  if(!to_delete.empty()) {
    for(auto &window : to_delete) {
      ImGui_ImplGlfwGL3_Shutdown();

      glfwDestroyWindow(window->_glfw_window);

      delete window;

      _windows.remove(window);
    }
  }

  return _windows.empty();
}

void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *param) {
  switch(severity) {
    case GL_DEBUG_SEVERITY_HIGH: {
        fprintf(stderr, "GL_DEBUG_SEVERITY_HIGH: %s type = 0x%x, message = %s\n", (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, message);
      }
      break;
    case GL_DEBUG_SEVERITY_MEDIUM: {
        fprintf(stderr, "GL_DEBUG_SEVERITY_MEDIUM: %s type = 0x%x, message = %s\n", (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, message);
      }
      break;
    case GL_DEBUG_SEVERITY_LOW: {
        fprintf(stderr, "GL_DEBUG_SEVERITY_LOW: %s type = 0x%x, message = %s\n", (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, message);
      }
      break;
    default:
      break;
  }
}


void init_render_states() {
  color_blending_state_ = device_->create_blend_state(true, scm::gl::FUNC_ONE, scm::gl::FUNC_ONE, scm::gl::FUNC_ONE,
                                                      scm::gl::FUNC_ONE, scm::gl::EQ_FUNC_ADD, scm::gl::EQ_FUNC_ADD);
  color_no_blending_state_ = device_->create_blend_state(false);

  depth_state_less_ = device_->create_depth_stencil_state(true, true, scm::gl::COMPARISON_LESS);
  auto no_depth_test_descriptor = depth_state_less_->descriptor();
  no_depth_test_descriptor._depth_test = false;
  depth_state_disable_ = device_->create_depth_stencil_state(no_depth_test_descriptor);
  depth_state_without_writing_ = device_->create_depth_stencil_state(true, false, scm::gl::COMPARISON_LESS_EQUAL);

  no_backface_culling_rasterizer_state_ = device_->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, false, false, 0.0, false, false);

  filter_linear_  = device_->create_sampler_state(scm::gl::FILTER_ANISOTROPIC, scm::gl::WRAP_CLAMP_TO_EDGE, 16u);
  filter_nearest_ = device_->create_sampler_state(scm::gl::FILTER_MIN_MAG_LINEAR, scm::gl::WRAP_CLAMP_TO_EDGE);
}


void init_camera() {
  if (settings_.use_view_tf_) {
    camera_ = new lamure::ren::camera();
    camera_->set_view_matrix(settings_.view_tf_);
    std::cout << "view_tf:" << std::endl;
    std::cout << camera_->get_high_precision_view_matrix() << std::endl;
    camera_->set_dolly_sens_(settings_.travel_speed_);
  }
  else {
    auto root_bb = lamure::ren::model_database::get_instance()->get_model(0)->get_bvh()->get_bounding_boxes()[0];
    auto root_bb_min = scm::math::mat4f(model_transformations_[0]) * root_bb.min_vertex();
    auto root_bb_max = scm::math::mat4f(model_transformations_[0]) * root_bb.max_vertex();
    scm::math::vec3f center = (root_bb_min + root_bb_max) / 2.f;

    camera_ = new lamure::ren::camera(0,
                                      make_look_at_matrix(center + scm::math::vec3f(0.f, 0.1f, -0.01f), center, scm::math::vec3f(0.f, 1.f, 0.f)),
                                      length(root_bb_max-root_bb_min), false, false);
    camera_->set_dolly_sens_(settings_.travel_speed_);
  }
  camera_->set_projection_matrix(settings_.fov_, float(settings_.width_)/float(settings_.height_),  settings_.near_plane_, settings_.far_plane_);

  screen_quad_.reset(new scm::gl::quad_geometry(device_, scm::math::vec2f(-1.0f, -1.0f), scm::math::vec2f(1.0f, 1.0f)));
}


void init_shaders() {
    shader_manager_ = std::make_shared<shader_manager>(device_);

    shader_manager_->add(VIS_QUAD_SHADER     , std::string("/vis/vis_quad.glslv")        , "/vis/vis_quad.glslf"                                 );
    shader_manager_->add(VIS_LINE_SHADER     , std::string("/vis/vis_line.glslv")        , "/vis/vis_line.glslf"                                 );
    shader_manager_->add(VIS_TRIANGLE_SHADER , std::string("/vis/vis_triangle.glslv")    , "/vis/vis_triangle.glslf"                             );
    shader_manager_->add(VIS_XYZ_SHADER      , std::string("/vis/vis_xyz.glslv")         , "/vis/vis_xyz.glslf"      , "/vis/vis_xyz.glslg"      );
    shader_manager_->add(VIS_XYZ_PASS1_SHADER, std::string("/vis/vis_xyz_pass1.glslv")   , "/vis/vis_xyz_pass1.glslf", "/vis/vis_xyz_pass1.glslg");
    shader_manager_->add(VIS_XYZ_PASS2_SHADER, std::string("/vis/vis_xyz_pass2.glslv")   , "/vis/vis_xyz_pass2.glslf", "/vis/vis_xyz_pass2.glslg");
    shader_manager_->add(VIS_XYZ_PASS3_SHADER, std::string("/vis/vis_xyz_pass3.glslv")   , "/vis/vis_xyz_pass3.glslf"                            );
    shader_manager_->add(VIS_VT_SHADER       , std::string("/vt/virtual_texturing.glslv"), "/vt/virtual_texturing_hierarchical.glslf"            );

    shader_manager_->add(VIS_XYZ_LIGHTING_SHADER      , true, "/vis/vis_xyz.glslv"         , "/vis/vis_xyz.glslf"      , "/vis/vis_xyz.glslg"      );
    shader_manager_->add(VIS_XYZ_PASS2_LIGHTING_SHADER, true, "/vis/vis_xyz_pass2.glslv"   , "/vis/vis_xyz_pass2.glslf", "/vis/vis_xyz_pass2.glslg");
    shader_manager_->add(VIS_XYZ_PASS3_LIGHTING_SHADER, true, "/vis/vis_xyz_pass3.glslv"   , "/vis/vis_xyz_pass3.glslf"                            );
}


void init() {


  device_.reset(new scm::gl::render_device());
  if (!device_) {
    std::cout << "error creating device" << std::endl;
  }


  context_ = device_->main_context();
  if (!context_) {
    std::cout << "error creating context" << std::endl;
  }

  init_shaders();


  if (settings_.pvs_ != "") {
    std::cout << "pvs: " << settings_.pvs_ << std::endl;
    std::string pvs_grid_file_path = settings_.pvs_;
    pvs_grid_file_path.resize(pvs_grid_file_path.length() - 3);
    pvs_grid_file_path = pvs_grid_file_path + "grid";

    lamure::pvs::pvs_database* pvs = lamure::pvs::pvs_database::get_instance();
    pvs->load_pvs_from_file(pvs_grid_file_path, settings_.pvs_, false);
    pvs->activate(settings_.use_pvs_);
    std::cout << "use pvs: " << (int)pvs->is_activated() << std::endl;
  }

  if (!settings_.atlas_file_.empty()) {
    vt_system_ = new vt_system(device_, context_, settings_.atlas_file_);
    vt_system_->set_shader_program(shader_manager_->get_program(VIS_VT_SHADER));
  }

  create_aux_resources();
  create_framebuffers();

  init_render_states();

  gui_ = std::make_shared<gui>();

  init_camera();

  if (settings_.background_image_ != "") {
    //std::cout << "background image: " << settings_.background_image_ << std::endl;
    scm::gl::texture_loader tl;
    bg_texture_ = tl.load_texture_2d(*device_, settings_.background_image_, true, false);
  }
}



int main(int argc, char *argv[])
{
    
  std::string vis_file = "";
  if (argc == 2) {
    vis_file = std::string(argv[1]);
  }
  else {
    std::cout << "Usage: " << argv[0] << " <vis_file.vis>" << std::endl;
    std::cout << "\tHELP: to render a single model use:" << std::endl;
    std::cout << "\techo <input_file.bvh> > default.vis && " << argv[0] << " default.vis" << std::endl;
    return 0;
  }

  putenv((char *)"__GL_SYNC_TO_VBLANK=0");

  settings_.load(vis_file);

  settings_.vis_ = settings_.show_normals_ ? 1
    : settings_.show_accuracy_ ? 2
    : settings_.show_output_sensitivity_ ? 3
    : settings_.channel_ > 0 ? 3+settings_.channel_
    : 0;

  if (settings_.provenance_ && settings_.json_ != "") {
    std::cout << "json: " << settings_.json_ << std::endl;
    data_provenance_ = lamure::ren::Data_Provenance::parse_json(settings_.json_);
    std::cout << "size of provenance: " << data_provenance_.get_size_in_bytes() << std::endl;
  }
 
  lamure::ren::policy* policy = lamure::ren::policy::get_instance();
  policy->set_max_upload_budget_in_mb(settings_.upload_);
  policy->set_render_budget_in_mb(settings_.vram_);
  policy->set_out_of_core_budget_in_mb(settings_.ram_);
  render_width_ = settings_.width_ / settings_.frame_div_;
  render_height_ = settings_.height_ / settings_.frame_div_;
  policy->set_window_width(settings_.width_);
  policy->set_window_height(settings_.height_);

  lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
  
  num_models_ = 0;
  for (const auto& input_file : settings_.models_) {
    lamure::model_t model_id = database->add_model(input_file, std::to_string(num_models_));
    model_transformations_.push_back(settings_.transforms_[num_models_] * scm::math::mat4d(scm::math::make_translation(database->get_model(num_models_)->get_bvh()->get_translation())));
    ++num_models_;
  }

  glfwSetErrorCallback(EventHandler::on_error);

  if (!glfwInit()) {
    std::runtime_error("GLFW initialisation failed");
  }

  Window *primary_window = create_window(settings_.width_, settings_.height_, "lamure_vis_gui", nullptr, nullptr);
  make_context_current(primary_window);
  glfwSwapInterval(1);

  init();

  make_context_current(primary_window);
  glut_display();

  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if (GLEW_OK != err) {
    std::cout << "GLEW error: " << glewGetErrorString(err) << std::endl;
  }
  std::cout << "using GLEW " << glewGetString(GLEW_VERSION) << std::endl;

  ImGui_ImplGlfwGL3_Init(primary_window->_glfw_window, false);
  ImGui_ImplGlfwGL3_CreateDeviceObjects();

  while(!should_close()) {
    glfwPollEvents();

    for(const auto &window : _windows) {
      make_context_current(window);

      if (window == primary_window) {
        glut_display();               

        if (settings_.gui_) {
          ImGui_ImplGlfwGL3_NewFrame();
          gui_->status_screen(settings_, input_, selection_, data_provenance_, num_models_, fps_, rendered_splats_, rendered_nodes_);
          ImGui::Render();
        }

      }
      glfwSwapBuffers(window->_glfw_window);
    }
  }

  return EXIT_SUCCESS;
}
