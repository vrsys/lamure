// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

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


//schism
#include <scm/core.h>
#include <scm/core/math.h>
#include <scm/core/io/tools.h>
#include <scm/core/pointer_types.h>
#include <scm/gl_core/gl_core_fwd.h>
#include <scm/gl_util/primitives/primitives_fwd.h>
#include <scm/core/platform/platform.h>
#include <scm/core/utilities/platform_warning_disable.h>
#include <scm/gl_core/render_device/opengl/gl_core.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/font/font_face.h>
#include <scm/gl_util/font/text.h>
#include <scm/gl_util/font/text_renderer.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>

#include <GL/freeglut.h>

//boost
#include <boost/assign/list_of.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>

bool rendering_ = false;
int32_t render_width_ = 1280;
int32_t render_height_ = 720;

int32_t num_models_ = 0;
std::vector<scm::math::mat4d> model_transformations_;

float height_divided_by_top_minus_bottom_ = 0.f;


lamure::ren::camera* camera_ = nullptr;

scm::shared_ptr<scm::gl::render_device> device_;
scm::shared_ptr<scm::gl::render_context> context_;

scm::gl::program_ptr vis_xyz_shader_;
scm::gl::program_ptr vis_xyz_pass1_shader_;
scm::gl::program_ptr vis_xyz_pass2_shader_;
scm::gl::program_ptr vis_xyz_pass3_shader_;

scm::gl::program_ptr vis_xyz_lighting_shader_;
scm::gl::program_ptr vis_xyz_pass2_lighting_shader_;
scm::gl::program_ptr vis_xyz_pass3_lighting_shader_;

scm::gl::program_ptr vis_xyz_qz_shader_;
scm::gl::program_ptr vis_xyz_qz_pass1_shader_;
scm::gl::program_ptr vis_xyz_qz_pass2_shader_;

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

scm::gl::texture_2d_ptr gaussian_texture_;

scm::gl::depth_stencil_state_ptr depth_state_disable_;
scm::gl::depth_stencil_state_ptr depth_state_less_;
scm::gl::rasterizer_state_ptr no_backface_culling_rasterizer_state_;
scm::gl::rasterizer_state_ptr change_point_size_in_shader_state_;

scm::gl::blend_state_ptr color_blending_state_;
scm::gl::blend_state_ptr color_no_blending_state_;

scm::gl::sampler_state_ptr filter_linear_;
scm::gl::sampler_state_ptr filter_nearest_;

scm::gl::buffer_ptr brush_buffer_;
scm::gl::vertex_array_ptr brush_array_;
scm::gl::program_ptr brush_shader_;

scm::gl::program_ptr quad_shader_;
scm::shared_ptr<scm::gl::quad_geometry> screen_quad_;
scm::gl::text_renderer_ptr text_renderer_;
scm::gl::text_ptr renderable_text_;
std::string text_ = "";
scm::time::accum_timer<scm::time::high_res_timer> frame_time_;
double fps_ = 0.0;
uint64_t rendered_splats_ = 0;
uint64_t rendered_nodes_ = 0;

lamure::ren::Data_Provenance data_provenance_;

struct input {
  float trackball_x_ = 0.f;
  float trackball_y_ = 0.f;
  scm::math::vec2i mouse_;
  scm::math::vec2i prev_mouse_;
  int32_t brush_mode_ = 0;
  lamure::ren::camera::mouse_state mouse_state_;
};

input input_;

struct gui {
  scm::math::mat4f ortho_matrix_;
};

gui gui_;

struct xyz {
  scm::math::vec3f pos_;
  char r_;
  char g_;
  char b_;
  char a_;
  float rad_;
  scm::math::vec3f nml_;
};

struct selection {
  int32_t selected_model_ = -1;
  int32_t num_strokes_ = 0;
  std::vector<xyz> brush_;
};

selection selection_;

struct settings {
  int32_t width_ {1920};
  int32_t height_ {1080};
  int32_t frame_div_ {1};
  int32_t vram_ {2048};
  int32_t ram_ {4096};
  int32_t upload_ {32};
  int32_t prov_ {0};
  float near_plane_ {0.001f};
  float far_plane_ {1000.0f};
  float fov_ {30.0f};
  int32_t splatting_ {1};
  int32_t gamma_correction_ {1};
  int32_t info_ {1};
  int32_t travel_ {2};
  float travel_speed_ {20.5f};
  int32_t lod_update_ {1};
  int32_t pvs_cull_ {0};
  int32_t vis_ {0};
  int32_t show_normals_ {0};
  int32_t show_accuracy_ {0};
  int32_t show_output_sensitivity_ {0};
  int32_t channel_ {0};
  float point_scale_ {1.0f};
  float error_threshold_ {LAMURE_DEFAULT_THRESHOLD};
  int32_t enable_lighting_ {1};
  int32_t use_material_color_ {0};
  scm::math::vec3f material_diffuse_ {0.6f, 0.6f, 0.6f};
  scm::math::vec4f material_specular_ {0.4f, 0.4f, 0.4f, 1000.0f};
  scm::math::vec3f ambient_light_color_ {0.1f, 0.1f, 0.1f};
  scm::math::vec4f point_light_color_ {1.0f, 1.0f, 1.0f, 1.2f};
  int32_t heatmap_ {0};
  float heatmap_min_ {0.0f};
  float heatmap_max_ {0.05f};
  scm::math::vec3f background_color_ {LAMURE_DEFAULT_COLOR_R, LAMURE_DEFAULT_COLOR_G, LAMURE_DEFAULT_COLOR_B};
  scm::math::vec3f heatmap_color_min_ {68.0f/255.0f, 0.0f, 84.0f/255.0f};
  scm::math::vec3f heatmap_color_max_ {251.f/255.f, 231.f/255.f, 35.f/255.f};
  std::string json_ {""};
  std::string pvs_ {""};
  std::vector<std::string> models_ {std::vector<std::string>()};
  std::vector<scm::math::mat4d> transforms_ {std::vector<scm::math::mat4d>()};

};

float brush_default_size_ = 0.002f;
float brush_default_offset_ = brush_default_size_;

settings settings_;

scm::math::mat4f load_matrix(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
      std::cerr << "Unable to open transformation file: \"" 
          << filename << "\"\n";
      return scm::math::mat4f::identity();
  }
  scm::math::mat4f mat = scm::math::mat4f::identity();
  std::string matrix_values_string;
  std::getline(file, matrix_values_string);
  std::stringstream sstr(matrix_values_string);
  for (int i = 0; i < 16; ++i)
      sstr >> mat[i];
  file.close();
  return scm::math::transpose(mat);
}


std::string const strip_whitespace(std::string const& in_string) {
  return boost::regex_replace(in_string, boost::regex("^ +| +$|( ) +"), "$1");

}


void load_settings(std::string const& vis_file_name, settings& settings) {

  std::ifstream vis_file(vis_file_name.c_str());

  if (!vis_file.is_open()) {
    std::cout << "could not open vis file" << std::endl;
    exit(-1);
  }
  else {
    lamure::model_t model_id = 0;

    std::string line;
    while(std::getline(vis_file, line)) {
      if(line.length() >= 2) {
        if (line[0] == '#') {
          continue;
        }
        auto colon = line.find_first_of(':');
        if (colon == std::string::npos) {
          scm::math::mat4d transform = scm::math::mat4d::identity();
          std::string model;

          std::istringstream line_ss(line);
          line_ss >> model;

          if (line_ss.rdbuf()->in_avail() != 0) {
            std::string tf_file;
            line_ss >> tf_file;
            tf_file = strip_whitespace(tf_file);
            if (tf_file.size() > 0) {
              if (tf_file.substr(tf_file.size()-3) == ".tf") {
                auto vis_filepath = boost::filesystem::canonical(vis_file_name);
                auto tf_filepath = boost::filesystem::absolute(boost::filesystem::path(tf_file), vis_filepath.parent_path());
                transform = load_matrix(tf_filepath.string());
              }
              else {
                std::cout << "unsupported transformation file" << std::endl;

              }
            }
          }
        
          settings.models_.push_back(model);
          settings.transforms_.push_back(transform);

        }
        else {
          std::string key = strip_whitespace(line.substr(0, colon));
          std::string value = strip_whitespace(line.substr(colon+1));
          if (key == "width") {
            settings.width_ = std::max(atoi(value.c_str()), 64);
          }
          else if (key == "height") {
            settings.height_ = std::max(atoi(value.c_str()), 64);
          }
          else if (key == "frame_div") {
            settings.frame_div_ = std::max(atoi(value.c_str()), 1);
          }
          else if (key == "vram") {
            settings.vram_ = std::max(atoi(value.c_str()), 8);
          }
          else if (key == "ram") {
            settings.ram_ = std::max(atoi(value.c_str()), 8);
          }
          else if (key == "upload") {
            settings.upload_ = std::max(atoi(value.c_str()), 8);
          }
          else if (key == "near") {
            settings.near_plane_ = std::max(atof(value.c_str()), 0.0);
          }
          else if (key == "far") {
            settings.far_plane_ = std::max(atof(value.c_str()), 0.1);
          }
          else if (key == "fov") {
            settings.fov_ = std::max(atof(value.c_str()), 9.0);
          }
          else if (key == "splatting") {
            settings.splatting_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "gamma_correction") {
            settings.gamma_correction_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "info") {
            settings.info_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "speed") {
            settings.travel_speed_ = std::min(std::max(atof(value.c_str()), 0.0), 400.0);
          }
          else if (key == "pvs_cull") {
            settings.pvs_cull_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "provenance") {
            settings.prov_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "show_normals") {
            settings.show_normals_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "show_accuracy") {
            settings.show_accuracy_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "show_output_sensitivity") {
            settings.show_output_sensitivity_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "channel") {
            settings.channel_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "point_scale") {
            settings.point_scale_ = std::min(std::max(atof(value.c_str()), 0.0), 10.0);
          }
          else if (key == "error") {
            settings.error_threshold_ = std::min(std::max(atof(value.c_str()), 0.0), 10.0);
          }
          else if (key == "enable_lighting") {
            settings.enable_lighting_ = std::min(std::max(atoi(value.c_str()), 0), 1);
          }
          else if (key == "use_material_color") {
            settings.use_material_color_ = std::min(std::max(atoi(value.c_str()), 0), 1);
          }
          else if (key == "material_diffuse_r") {
            settings.material_diffuse_.x = std::max(atof(value.c_str()), 0.0);
          }
          else if (key == "material_diffuse_g") {
            settings.material_diffuse_.y = std::max(atof(value.c_str()), 0.0);
          }
          else if (key == "material_diffuse_b") {
            settings.material_diffuse_.z = std::max(atof(value.c_str()), 0.0);
          }
          else if (key == "material_specular_r") {
            settings.material_specular_.x = std::max(atof(value.c_str()), 0.0);
          }
          else if (key == "material_specular_g") {
            settings.material_specular_.y = std::max(atof(value.c_str()), 0.0);
          }
          else if (key == "material_specular_b") {
            settings.material_specular_.z = std::max(atof(value.c_str()), 0.0);
          }
          else if (key == "material_specular_exponent") {
            settings.material_specular_.w = std::min(std::max(atof(value.c_str()), 0.0), 10000.0);
          }
          else if (key == "ambient_light_color_r") {
            settings.ambient_light_color_.r = std::min(std::max(atof(value.c_str()), 0.0), 1.0);
          }
          else if (key == "ambient_light_color_g") {
            settings.ambient_light_color_.g = std::min(std::max(atof(value.c_str()), 0.0), 1.0);
          }
          else if (key == "ambient_light_color_b") {
            settings.ambient_light_color_.b = std::min(std::max(atof(value.c_str()), 0.0), 1.0);
          }
          else if (key == "point_light_color_r") {
            settings.point_light_color_.r = std::min(std::max(atof(value.c_str()), 0.0), 1.0);
          }
          else if (key == "point_light_color_g") {
            settings.point_light_color_.g = std::min(std::max(atof(value.c_str()), 0.0), 1.0);
          }
          else if (key == "point_light_color_b") {
            settings.point_light_color_.b = std::min(std::max(atof(value.c_str()), 0.0), 1.0);
          }
          else if (key == "point_light_intensity") {
            settings.point_light_color_.w = std::min(std::max(atof(value.c_str()), 0.0), 10000.0);
          }
          else if (key == "background_color_r") {
            settings.background_color_.x = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
          }
          else if (key == "background_color_g") {
            settings.background_color_.y = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
          }
          else if (key == "background_color_b") {
            settings.background_color_.z = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
          }
          else if (key == "heatmap") {
            settings.heatmap_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "heatmap_min") {
            settings.heatmap_min_ = std::max(atof(value.c_str()), 0.0);
          }
          else if (key == "heatmap_max") {
            settings.heatmap_max_ = std::max(atof(value.c_str()), 0.0);
          }
          else if (key == "heatmap_min_r") {
            settings.heatmap_color_min_.x = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
          }
          else if (key == "heatmap_min_g") {
            settings.heatmap_color_min_.y = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
          }
          else if (key == "heatmap_min_b") {
            settings.heatmap_color_min_.z = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
          }
          else if (key == "heatmap_max_r") {
            settings.heatmap_color_max_.x = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
          }
          else if (key == "heatmap_max_g") {
            settings.heatmap_color_max_.y = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
          }
          else if (key == "heatmap_max_b") {
            settings.heatmap_color_max_.z = std::min(std::max(atoi(value.c_str()), 0), 255)/255.f;
          }
          else if (key == "json") {
            settings.json_ = value;
          }
          else if (key == "pvs") {
            settings.pvs_ = value;
          }

          //std::cout << key << " : " << value << std::endl;
        }

      }
    }
    vis_file.close();
  }

  //assertions
  if (settings.prov_ != 0) {
    if (settings.json_ == "") {
      std::cout << "error: pls provide a provenance json description or set prov to 0" << std::endl;
      exit(-1);
    }
    if (settings.json_.size() > 0) {
      if (settings.json_.substr(settings.json_.size()-5) != ".json") {
        std::cout << "unsupported json file" << std::endl;
        exit(-1);
      }
    }
  }
  if (settings.models_.empty()) {
    std::cout << "error: no model filename specified" << std::endl;
    exit(-1);
  }
  if (settings.pvs_.size() > 0) {
    if (settings.pvs_.substr(settings.pvs_.size()-4) != ".pvs") {
      std::cout << "unsupported pvs file" << std::endl;
      exit(-1);
    }
  }


}

char* get_cmd_option(char** begin, char** end, const std::string& option) {
  char** it = std::find(begin, end, option);
  if (it != end && ++it != end) {
    return *it;
  }
  return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
  return std::find(begin, end, option) != end;
}


void draw_brush(scm::gl::program_ptr shader) {

  if (selection_.num_strokes_ > 0) {

    //draw brush surfels
    context_->bind_vertex_array(brush_array_);

    scm::math::mat4d model_matrix = scm::math::mat4d::identity();
    scm::math::mat4d projection_matrix = scm::math::mat4d(camera_->get_projection_matrix());
    scm::math::mat4d view_matrix = camera_->get_high_precision_view_matrix();
    scm::math::mat4d model_view_matrix = view_matrix * model_matrix;
    scm::math::mat4d model_view_projection_matrix = projection_matrix * model_view_matrix;

    //uniforms for brush
    shader->uniform("mvp_matrix", scm::math::mat4f(model_view_projection_matrix));
    shader->uniform("model_view_matrix", scm::math::mat4f(model_view_matrix));
    shader->uniform("inv_mv_matrix", scm::math::mat4f(scm::math::transpose(scm::math::inverse(model_view_matrix))));

    shader->uniform("point_size_factor", settings_.point_scale_);
    shader->uniform("show_normals", false);
    shader->uniform("show_accuracy", false);
    shader->uniform("show_output_sensitivity", false);
    shader->uniform("channel", 0);

    context_->apply();

    context_->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST, 0, selection_.num_strokes_);
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
    lamure::model_t m_id = controller->deduce_model_id(std::to_string(model_id));
    lamure::ren::cut& cut = cuts->get_cut(context_id, view_id, m_id);
    std::vector<lamure::ren::cut::node_slot_aggregate> renderable = cut.complete_set();
    const lamure::ren::bvh* bvh = database->get_model(m_id)->get_bvh();
    if (bvh->get_primitive() != lamure::ren::bvh::primitive_type::POINTCLOUD) {
      continue;
    }
    
    //uniforms per model
    scm::math::mat4d model_matrix = model_transformations_[model_id];
    scm::math::mat4d projection_matrix = scm::math::mat4d(camera_->get_projection_matrix());
    scm::math::mat4d view_matrix = camera_->get_high_precision_view_matrix();
    scm::math::mat4d model_view_matrix = view_matrix * model_matrix;
    scm::math::mat4d model_view_projection_matrix = projection_matrix * model_view_matrix;

    shader->uniform("mvp_matrix", scm::math::mat4f(model_view_projection_matrix));
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

        if (pvs->is_activated() && settings_.pvs_cull_ 
          && !lamure::pvs::pvs_database::get_instance()->get_viewer_visibility(model_id, node_slot_aggregate.node_id_)) {
          continue;
        }
        
        if (settings_.show_accuracy_) {
          const float accuracy = 1.0 - (bvh->get_depth_of_node(node_slot_aggregate.node_id_) * 1.0)/(bvh->get_depth() - 1);
          shader->uniform("accuracy", accuracy);
        }

        context_->apply();

        context_->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST,
          (node_slot_aggregate.slot_id_) * (GLsizei)surfels_per_node, surfels_per_node);

        rendered_splats_ += surfels_per_node;
        ++rendered_nodes_;
      
      }
    }
    if (selection_.selected_model_ != -1) {
      break;
    }
  }

}

void set_uniforms(scm::gl::program_ptr shader) {
  shader->uniform("win_size", scm::math::vec2f(render_width_, render_height_));

  shader->uniform("height_divided_by_top_minus_bottom", height_divided_by_top_minus_bottom_);
  shader->uniform("near_plane", settings_.near_plane_);
  shader->uniform("far_minus_near_plane", settings_.far_plane_-settings_.near_plane_);
  shader->uniform("point_size_factor", settings_.point_scale_);

  shader->uniform("ellipsify", true);
  shader->uniform("clamped_normal_mode", true);
  shader->uniform("max_deform_ratio", 0.35f);

  shader->uniform("show_normals", (bool)settings_.show_normals_);
  shader->uniform("show_accuracy", (bool)settings_.show_accuracy_);
  shader->uniform("show_output_sensitivity", (bool)settings_.show_output_sensitivity_);

  shader->uniform("channel", settings_.channel_);
  shader->uniform("heatmap", (bool)settings_.heatmap_);

  shader->uniform("heatmap_min", settings_.heatmap_min_);
  shader->uniform("heatmap_max", settings_.heatmap_max_);
  shader->uniform("heatmap_min_color", settings_.heatmap_color_min_);
  shader->uniform("heatmap_max_color", settings_.heatmap_color_max_);
}

void set_lighting_uniforms(scm::gl::program_ptr shader) {

  shader->uniform("use_material_color", settings_.use_material_color_);
  shader->uniform("material_diffuse", settings_.material_diffuse_);
  shader->uniform("material_specular", settings_.material_specular_);

  shader->uniform("ambient_light_color", settings_.ambient_light_color_);
  shader->uniform("point_light_color", settings_.point_light_color_);
}

void create_brush() {

  if (selection_.brush_.empty()) {
    selection_.num_strokes_ = 0;
    return;
  }
  if (selection_.num_strokes_ == selection_.brush_.size()) {
    return;
  }

  selection_.num_strokes_ = selection_.brush_.size();
  
  brush_buffer_.reset();
  brush_array_.reset(); 

  brush_buffer_ = device_->create_buffer(
    scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, sizeof(xyz) * selection_.num_strokes_, &selection_.brush_[0]);
  brush_array_ = device_->create_vertex_array(scm::gl::vertex_format
    (0, 0, scm::gl::TYPE_VEC3F, sizeof(xyz))
    (0, 1, scm::gl::TYPE_UBYTE, sizeof(xyz), scm::gl::INT_FLOAT_NORMALIZE)
    (0, 2, scm::gl::TYPE_UBYTE, sizeof(xyz), scm::gl::INT_FLOAT_NORMALIZE)
    (0, 3, scm::gl::TYPE_UBYTE, sizeof(xyz), scm::gl::INT_FLOAT_NORMALIZE)
    (0, 4, scm::gl::TYPE_UBYTE, sizeof(xyz), scm::gl::INT_FLOAT_NORMALIZE)
    (0, 5, scm::gl::TYPE_FLOAT, sizeof(xyz))
    (0, 6, scm::gl::TYPE_VEC3F, sizeof(xyz)),
    boost::assign::list_of(brush_buffer_));

}

void glut_display() {
  if (rendering_) {
    return;
  }
  rendering_ = true;

  lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
  lamure::ren::cut_database* cuts = lamure::ren::cut_database::get_instance();
  lamure::ren::controller* controller = lamure::ren::controller::get_instance();
  lamure::pvs::pvs_database* pvs = lamure::pvs::pvs_database::get_instance();
  
  if (settings_.info_) {
    std::ostringstream text_ss;
    text_ss << "fps: " << (int32_t)fps_ << "\n";
    text_ss << "# points: " << std::setprecision(2) << (rendered_splats_ / 1000000.0) << " mio.\n";
    text_ss << "# nodes: " << std::to_string(rendered_nodes_) << "\n";
    text_ss << "\n";
    text_ss << "vis (e/E): " << settings_.vis_ << "\n";
    text_ss << "lighting (l): " << settings_.enable_lighting_ << "\n";
    text_ss << "use point color for lighting (c): " << !settings_.use_material_color_ << "\n";

    if (selection_.selected_model_ == -1) {
      text_ss << "datasets: " << num_models_ << "\n";
    }
    else {
      text_ss << "dataset: " << selection_.selected_model_+1 << " / " << num_models_ << "\n";
    }
    text_ss << "brush (b): " << input_.brush_mode_ << "\n";
    text_ss << "\n";
    text_ss << "lod_update (d): " << settings_.lod_update_ << "\n";
    text_ss << "splatting (q): " << settings_.splatting_ << "\n";
    text_ss << "pvs (p): " << pvs->is_activated() << "\n";
    text_ss << "point_scale (u/j): " << std::setprecision(2) << settings_.point_scale_ << "\n";
    text_ss << "error (i/k): " << std::setprecision(2) << settings_.error_threshold_ << "\n";
    text_ss << "speed (f/F): " << std::setprecision(3) << settings_.travel_speed_ << "\n";
    text_ = text_ss.str();
  }

  bool signal_shutdown = false;
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
    cuts->send_threshold(context_id, m_id, settings_.error_threshold_);
    cuts->send_rendered(context_id, m_id);
    
    database->get_model(m_id)->set_transform(scm::math::mat4f(model_transformations_[m_id]));
  }


  lamure::view_t cam_id = controller->deduce_view_id(context_id, camera_->view_id());
  cuts->send_camera(context_id, cam_id, *camera_);

  std::vector<scm::math::vec3d> corner_values = camera_->get_frustum_corners();
  double top_minus_bottom = scm::math::length((corner_values[2]) - (corner_values[0]));
  height_divided_by_top_minus_bottom_ = lamure::ren::policy::get_instance()->window_height() / top_minus_bottom;

  cuts->send_height_divided_by_top_minus_bottom(context_id, cam_id, height_divided_by_top_minus_bottom_);

  if (true) {
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
 
  
  create_brush();

  if (settings_.splatting_) {
    //2 pass splatting
    //PASS 1

    context_->clear_color_buffer(pass1_fbo_ , 0, scm::math::vec4f( .0f, .0f, .0f, 0.0f));
    context_->clear_depth_stencil_buffer(pass1_fbo_);
    context_->set_frame_buffer(pass1_fbo_);
      
    
    context_->bind_program(vis_xyz_pass1_shader_);
    context_->set_blend_state(color_no_blending_state_);
    context_->set_rasterizer_state(change_point_size_in_shader_state_);
    context_->set_depth_stencil_state(depth_state_less_);
    
    vis_xyz_pass1_shader_->uniform("near_plane", settings_.near_plane_);
    vis_xyz_pass1_shader_->uniform("point_size_factor", settings_.point_scale_);
    vis_xyz_pass1_shader_->uniform("height_divided_by_top_minus_bottom", height_divided_by_top_minus_bottom_);
    vis_xyz_pass1_shader_->uniform("far_minus_near_plane", settings_.far_plane_-settings_.near_plane_);

    vis_xyz_pass1_shader_->uniform("ellipsify", true);
    vis_xyz_pass1_shader_->uniform("clamped_normal_mode", true);
    vis_xyz_pass1_shader_->uniform("max_deform_ratio", 0.35f);

    context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(render_width_, render_height_)));
    context_->apply();

    draw_all_models(context_id, view_id, vis_xyz_pass1_shader_);

    draw_brush(vis_xyz_pass1_shader_);

    //PASS 2

    context_->clear_color_buffer(pass2_fbo_ , 0, scm::math::vec4f( .0f, .0f, .0f, 0.0f));

    if(settings_.enable_lighting_) {
      context_->clear_color_buffer(pass2_fbo_ , 1, scm::math::vec4f( .0f, .0f, .0f, 0.0f));
      context_->clear_color_buffer(pass2_fbo_ , 2, scm::math::vec4f( .0f, .0f, .0f, 0.0f));
    }
    context_->set_frame_buffer(pass2_fbo_);

    context_->set_blend_state(color_blending_state_);
    context_->set_depth_stencil_state(depth_state_disable_);

    auto selected_pass2_shading_program = vis_xyz_pass2_shader_;

    if(settings_.enable_lighting_) {
      selected_pass2_shading_program = vis_xyz_pass2_lighting_shader_;
    }

    context_->bind_program(selected_pass2_shading_program);
    
    selected_pass2_shading_program->uniform("depth_texture_pass1", 0);
    selected_pass2_shading_program->uniform("pointsprite_texture", 1);

    context_->bind_texture(pass1_depth_buffer_, filter_nearest_, 0);
    context_->bind_texture(gaussian_texture_, filter_nearest_, 1);

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

    auto selected_pass3_shading_program = vis_xyz_pass3_shader_;

    if(settings_.enable_lighting_) {
      selected_pass3_shading_program = vis_xyz_pass3_lighting_shader_;
    }

    context_->bind_program(selected_pass3_shading_program);

    if(settings_.enable_lighting_) {
      set_lighting_uniforms(selected_pass3_shading_program);
    }

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

    auto selected_single_pass_shading_program = vis_xyz_shader_;

    if(settings_.enable_lighting_) {
      selected_single_pass_shading_program = vis_xyz_lighting_shader_;
    }


    context_->bind_program(selected_single_pass_shading_program);
    context_->set_rasterizer_state(change_point_size_in_shader_state_);
    context_->set_blend_state(color_no_blending_state_);
    context_->set_depth_stencil_state(depth_state_less_);
    
    set_uniforms(selected_single_pass_shading_program);

    if(settings_.enable_lighting_) {
      set_lighting_uniforms(selected_single_pass_shading_program);
    }
    context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(render_width_, render_height_)));
    context_->apply();

    draw_all_models(context_id, view_id, selected_single_pass_shading_program);

    draw_brush(selected_single_pass_shading_program);
  }


  //PASS 4: fullscreen quad
  
  context_->clear_default_depth_stencil_buffer();
  context_->clear_default_color_buffer();
  context_->set_default_frame_buffer();
  
  context_->bind_program(quad_shader_);
  
  context_->bind_texture(fbo_color_buffer_, filter_linear_, 0);
  quad_shader_->uniform("gamma_correction", (bool)settings_.gamma_correction_);

  context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(settings_.width_, settings_.height_)));
  context_->apply();
  
  screen_quad_->draw(context_);

  if (settings_.info_) {
    renderable_text_->text_string(text_);
    text_renderer_->draw_shadowed(context_, scm::math::vec2i(28, settings_.height_- 40), renderable_text_);
  }

  rendering_ = false;
  glutSwapBuffers();

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
        intersection.position_ + intersection.normal_ * brush_default_offset_,
        (char)color.x, (char)color.y, (char)color.z, (char)255,
        brush_default_size_,
        intersection.normal_});
  }

}

void create_framebuffers() {

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

  // begin: optional block
  pass2_normal_buffer_ = device_->create_texture_2d(scm::math::vec2ui(render_width_, render_height_), scm::gl::FORMAT_RGB_32F, 1, 1, 1);
  pass2_fbo_->attach_color_buffer(1, pass2_normal_buffer_);
  pass2_view_space_pos_buffer_ = device_->create_texture_2d(scm::math::vec2ui(render_width_, render_height_), scm::gl::FORMAT_RGB_32F, 1, 1, 1);
  pass2_fbo_->attach_color_buffer(2, pass2_view_space_pos_buffer_);
  // end: optional block

}


void glut_resize(int32_t w, int32_t h) {
  settings_.width_ = w;
  settings_.height_ = h;

  render_width_ = settings_.width_ / settings_.frame_div_;
  render_height_ = settings_.height_ / settings_.frame_div_;

  create_framebuffers();
  
  lamure::ren::policy* policy = lamure::ren::policy::get_instance();
  policy->set_window_width(render_width_);
  policy->set_window_height(render_height_);
  
  context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(render_width_, render_height_)));

  camera_->set_projection_matrix(settings_.fov_, float(settings_.width_)/float(settings_.height_),  settings_.near_plane_, settings_.far_plane_);

  gui_.ortho_matrix_ = 
    scm::math::make_ortho_matrix(0.0f, static_cast<float>(settings_.width_),
    0.0f, static_cast<float>(settings_.height_), -1.0f, 1.0f);

  
  text_renderer_->projection_matrix(gui_.ortho_matrix_);

  

}

void glut_keyboard(unsigned char key, int32_t x, int32_t y) {
  switch (key) {
    case 27:
      exit(0);
      break;

    case '.':
      glutFullScreenToggle();
      break;

    case 'q':
      settings_.splatting_ = !settings_.splatting_;
      break;

    case 'd':
      settings_.lod_update_ = !settings_.lod_update_;
      break;

    case 'e':
      ++settings_.vis_;
      if(settings_.vis_ > (3 + settings_.prov_/12)) {
        settings_.vis_ = 0;
      }
      settings_.show_normals_ = (settings_.vis_ == 1);
      settings_.show_accuracy_ = (settings_.vis_ == 2);
      settings_.show_output_sensitivity_ = (settings_.vis_ == 3);
      if (settings_.vis_ > 3) {
        settings_.channel_ = (settings_.vis_-3);
      }
      else {
        settings_.channel_ = 0;
      }
      break;

    case 'E':
      --settings_.vis_;
      if(settings_.vis_ < 0) {
        settings_.vis_ = (3 + settings_.prov_/12);
      }
      settings_.show_normals_ = (settings_.vis_ == 1);
      settings_.show_accuracy_ = (settings_.vis_ == 2);
      settings_.show_output_sensitivity_ = (settings_.vis_ == 3);
      if (settings_.vis_ > 3) {
        settings_.channel_ = (settings_.vis_-3);
      }
      else {
        settings_.channel_ = 0;
      }
      break;

    case 'h':
      settings_.heatmap_ = !settings_.heatmap_;
      break;

    case 'b':
      input_.brush_mode_ = !input_.brush_mode_;
      break;

    case 'p':
      {
        lamure::pvs::pvs_database* pvs = lamure::pvs::pvs_database::get_instance();
        pvs->activate(!pvs->is_activated());
        break;
      }

    case 'u':
      if (settings_.point_scale_ >= 0.2) {
        settings_.point_scale_ -= 0.1;
      }
      break;

    case 'j':
      if (settings_.point_scale_ < 1.9) {
        settings_.point_scale_ += 0.1;
      }
      break;


    case 'i':
      settings_.error_threshold_ -= 0.1f;
      if (settings_.error_threshold_ < LAMURE_MIN_THRESHOLD)
        settings_.error_threshold_ = LAMURE_MIN_THRESHOLD;
      break;

    case 'k':
      settings_.error_threshold_ += 0.1f;
      if (settings_.error_threshold_ > LAMURE_MAX_THRESHOLD)
        settings_.error_threshold_ = LAMURE_MAX_THRESHOLD;
      break;

    case 'l':
      settings_.enable_lighting_ = !settings_.enable_lighting_;
      break;

    case 'c':
      settings_.use_material_color_ = !settings_.use_material_color_;
      break;

    case 'f':
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
      
      case 'F':
      {
        if(settings_.travel_ > 0){
          --settings_.travel_;
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

    default:
      break;

  }

}


void glut_motion(int32_t x, int32_t y) {

  input_.prev_mouse_ = input_.mouse_;
  input_.mouse_ = scm::math::vec2i(x, y);
  
  if (!input_.brush_mode_) {
    camera_->update_trackball(x, y, settings_.width_, settings_.height_, input_.mouse_state_);
  }
  else {
    brush();
  }
}

void glut_mouse(int32_t button, int32_t state, int32_t x, int32_t y) {

  switch (button) {
    case GLUT_LEFT_BUTTON:
      input_.mouse_state_.lb_down_ = (state == GLUT_DOWN) ? true : false;
      break;
    case GLUT_MIDDLE_BUTTON:
      input_.mouse_state_.mb_down_ = (state == GLUT_DOWN) ? true : false;
      break;
    case GLUT_RIGHT_BUTTON:
      input_.mouse_state_.rb_down_ = (state == GLUT_DOWN) ? true : false;
      break;
    default: break;
  }

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


void glut_idle() {
  glutPostRedisplay();
}

//checks for prefix AND removes it (+ whitespace) if it is found; 
//returns true, if prefix was found; else false
bool parse_prefix(std::string& in_string, std::string const& prefix) {

 uint32_t num_prefix_characters = prefix.size();

 bool prefix_found 
  = (!(in_string.size() < num_prefix_characters ) 
     && strncmp(in_string.c_str(), prefix.c_str(), num_prefix_characters ) == 0); 

  if( prefix_found ) {
    in_string = in_string.substr(num_prefix_characters);
    in_string = strip_whitespace(in_string);
  }

  return prefix_found;
}

void resolve_relative_path(std::string& base_path, std::string& relative_path) {

    std::cout << "Starting parse prefix with relative path: " << relative_path << "\n";

  while(parse_prefix(relative_path, "../")) {
    std::cout << "Parse prefix true\n";
      size_t slash_position = base_path.find_last_of("/", base_path.size()-2);

      base_path = base_path.substr(0, slash_position);
  }
}

bool read_shader(std::string const& path_string, 
                 std::string& shader_string, bool keep_optional_shader_code = false) {


  if ( !boost::filesystem::exists( path_string ) ) {
    std::cout << "WARNING: File " << path_string << "does not exist." <<  std::endl;
    return false;
  }

  std::ifstream shader_source(path_string, std::ios::in);
  std::string line_buffer;

  std::string include_prefix("INCLUDE");

  std::string optional_begin_prefix("OPTIONAL_BEGIN");
  std::string optional_end_prefix("OPTIONAL_END");

  std::size_t slash_position = path_string.find_last_of("/\\");
  std::string const base_path =  path_string.substr(0,slash_position+1);

  bool disregard_code = false;

  while( std::getline(shader_source, line_buffer) ) {
    line_buffer = strip_whitespace(line_buffer);
    //std::cout << line_buffer << "\n";

    if( parse_prefix(line_buffer, include_prefix) ) {
      if(!disregard_code || keep_optional_shader_code) {
        std::string filename_string = line_buffer;
        read_shader(base_path+filename_string, shader_string);
      }
    } else if (parse_prefix(line_buffer, optional_begin_prefix)) {
      disregard_code = true;
    } else if (parse_prefix(line_buffer, optional_end_prefix)) {
      disregard_code = false;
    } 
    else {
      if( (!disregard_code) || keep_optional_shader_code ) {
        shader_string += line_buffer+"\n";
      }
    }
  }

  return true;
}


int32_t main(int argc, char* argv[]) {

  std::string vis_file = "";
  if (argc == 2) {
    vis_file = std::string(argv[1]);
  }
  else {
    std::cout << "Usage: " << argv[0] << " <vis_file.vis>\n" << 
      "\n";
    return 0;
  }

  putenv((char *)"__GL_SYNC_TO_VBLANK=0");

  load_settings(vis_file, settings_);

  settings_.vis_ = settings_.show_normals_ ? 1
    : settings_.show_accuracy_ ? 2
    : settings_.show_output_sensitivity_ ? 3
    : settings_.channel_ > 0 ? 3+settings_.channel_
    : 0;

 
  lamure::ren::policy* policy = lamure::ren::policy::get_instance();
  policy->set_max_upload_budget_in_mb(settings_.upload_);
  policy->set_render_budget_in_mb(settings_.vram_);
  policy->set_out_of_core_budget_in_mb(settings_.ram_);
  render_width_ = settings_.width_ / settings_.frame_div_;
  render_height_ = settings_.height_ / settings_.frame_div_;
  policy->set_window_width(settings_.width_);
  policy->set_window_height(settings_.height_);
  policy->set_size_of_provenance(settings_.prov_);

  if (policy->size_of_provenance() > 0) {
    data_provenance_ = lamure::ren::Data_Provenance::parse_json(settings_.json_);
  }

  lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
  
  num_models_ = 0;
  for (const auto& input_file : settings_.models_) {
    lamure::model_t model_id = database->add_model(input_file, std::to_string(num_models_));
    model_transformations_.push_back(settings_.transforms_[num_models_] * scm::math::mat4d(scm::math::make_translation(database->get_model(num_models_)->get_bvh()->get_translation())));
    ++num_models_;
  }
  
  std::cout << settings_.pvs_ << std::endl;
  if(settings_.pvs_ != "") {
    std::cout << "pvs: " << settings_.pvs_ << std::endl;
    std::string pvs_grid_file_path = settings_.pvs_;
    pvs_grid_file_path.resize(pvs_grid_file_path.length() - 3);
    pvs_grid_file_path = pvs_grid_file_path + "grid";

    lamure::pvs::pvs_database* pvs = lamure::pvs::pvs_database::get_instance();
    pvs->load_pvs_from_file(pvs_grid_file_path, settings_.pvs_, false);
  }
 
  glutInit(&argc, argv);
  glutInitContextVersion(4, 4);
  glutInitContextProfile(GLUT_CORE_PROFILE);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
  //glewExperimental = GL_TRUE;
  //glewInit();
  glutInitWindowSize(settings_.width_, settings_.height_);
  glutInitWindowPosition(64, 64);
  glutCreateWindow(argv[0]);
  glutSetWindowTitle("lamure_vis");
  
  glutDisplayFunc(glut_display);
  glutReshapeFunc(glut_resize);
  glutKeyboardFunc(glut_keyboard);
  glutMotionFunc(glut_motion);
  glutMouseFunc(glut_mouse);
  glutIdleFunc(glut_idle);

  device_.reset(new scm::gl::render_device());


  context_ = device_->main_context();
  
  try
  {
    std::string quad_shader_fs_source;
    std::string quad_shader_vs_source;
    
    std::string vis_xyz_vs_source;
    std::string vis_xyz_fs_source;
    
    std::string vis_xyz_pass1_vs_source;
    std::string vis_xyz_pass1_fs_source;
    std::string vis_xyz_pass2_vs_source;
    std::string vis_xyz_pass2_fs_source;
    std::string vis_xyz_pass3_vs_source;
    std::string vis_xyz_pass3_fs_source;


    std::string vis_xyz_qz_vs_source;
    std::string vis_xyz_qz_pass1_vs_source;
    std::string vis_xyz_qz_pass2_vs_source;

    /* parsed with optional lighting code */
    std::string vis_xyz_vs_lighting_source;
    std::string vis_xyz_fs_lighting_source;
    std::string vis_xyz_pass2_vs_lighting_source;
    std::string vis_xyz_pass2_fs_lighting_source;
    std::string vis_xyz_pass3_vs_lighting_source;
    std::string vis_xyz_pass3_fs_lighting_source;

    if (!read_shader("../share/lamure/shaders/vis/vis_quad.glslv", quad_shader_vs_source)
      || !read_shader("../share/lamure/shaders/vis/vis_quad.glslf", quad_shader_fs_source)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz.glslv", vis_xyz_vs_source)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz.glslf", vis_xyz_fs_source)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz_pass1.glslv", vis_xyz_pass1_vs_source)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz_pass1.glslf", vis_xyz_pass1_fs_source)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz_pass2.glslv", vis_xyz_pass2_vs_source)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz_pass2.glslf", vis_xyz_pass2_fs_source)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz_pass3.glslv", vis_xyz_pass3_vs_source)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz_pass3.glslf", vis_xyz_pass3_fs_source)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz_qz.glslv", vis_xyz_qz_vs_source)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz_qz_pass1.glslv", vis_xyz_qz_pass1_vs_source)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz_qz_pass2.glslv", vis_xyz_qz_pass2_vs_source)

      || !read_shader("../share/lamure/shaders/vis/vis_xyz.glslv", vis_xyz_vs_lighting_source, true)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz.glslf", vis_xyz_fs_lighting_source, true)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz_pass2.glslv", vis_xyz_pass2_vs_lighting_source, true)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz_pass2.glslf", vis_xyz_pass2_fs_lighting_source, true)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz_pass3.glslv", vis_xyz_pass3_vs_lighting_source, true)
      || !read_shader("../share/lamure/shaders/vis/vis_xyz_pass3.glslf", vis_xyz_pass3_fs_lighting_source, true)
      ) {
      std::cout << "error reading shader files" << std::endl;
      return 1;
    }

    quad_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, quad_shader_vs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, quad_shader_fs_source)));
    if (!quad_shader_) {
      std::cout << "error creating shader quad_shader_ program" << std::endl;
      return 1;
    }

    vis_xyz_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_vs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_fs_source)));
    if (!vis_xyz_shader_) {
      std::cout << "error creating shader vis_xyz_shader_ program" << std::endl;
      return 1;
    }

    vis_xyz_pass1_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_pass1_vs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_pass1_fs_source)));
    if (!vis_xyz_pass1_shader_) {
      std::cout << "error creating vis_xyz_pass1_shader_ program" << std::endl;
      return 1;
    }

    vis_xyz_pass2_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_pass2_vs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_pass2_fs_source)));
    if (!vis_xyz_pass2_shader_) {
      std::cout << "error creating vis_xyz_pass2_shader_ program" << std::endl;
      return 1;
    }

    vis_xyz_pass3_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_pass3_vs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_pass3_fs_source)));
    if (!vis_xyz_pass3_shader_) {
      std::cout << "error creating vis_xyz_pass3_shader_ program" << std::endl;
      return 1;
    }

    vis_xyz_lighting_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_vs_lighting_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_fs_lighting_source)));
    if (!vis_xyz_lighting_shader_) {
      std::cout << "error creating vis_xyz_lighting_shader_ program" << std::endl;
      return 1;
    }

    vis_xyz_pass2_lighting_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_pass2_vs_lighting_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_pass2_fs_lighting_source)));
    if (!vis_xyz_pass2_lighting_shader_) {
      std::cout << "error creating vis_xyz_pass2_lighting_shader_ program" << std::endl;
      return 1;
    }

    vis_xyz_pass3_lighting_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_pass3_vs_lighting_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_pass3_fs_lighting_source)));
    if (!vis_xyz_pass3_lighting_shader_) {
      std::cout << "error creating vis_xyz_pass3_lighting_shader_ program" << std::endl;
      return 1;
    }

    vis_xyz_qz_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_qz_vs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_fs_source)));
    if (!vis_xyz_qz_shader_) {
      std::cout << "error vis_xyz_qz_shader_ program" << std::endl;
      return 1;
    }

    vis_xyz_qz_pass1_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_qz_pass1_vs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_pass1_fs_source)));
    if (!vis_xyz_qz_pass1_shader_) {
      std::cout << "error vis_xyz_qz_pass1_shader program" << std::endl;
      return 1;
    }

    vis_xyz_qz_pass2_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_qz_pass2_vs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_pass2_fs_source)));
    if (!vis_xyz_qz_pass2_shader_) {
      std::cout << "error creating vis_xyz_qz_pass2_shader_ program" << std::endl;
      return 1;
    }

  }
  catch (std::exception& e)
  {
      std::cout << e.what() << std::endl;
  }


  glutShowWindow();
  
  create_framebuffers();

  color_blending_state_ = device_->create_blend_state(true, scm::gl::FUNC_ONE, scm::gl::FUNC_ONE, scm::gl::FUNC_ONE, 
    scm::gl::FUNC_ONE, scm::gl::EQ_FUNC_ADD, scm::gl::EQ_FUNC_ADD);
  color_no_blending_state_ = device_->create_blend_state(false);

  depth_state_less_ = device_->create_depth_stencil_state(true, true, scm::gl::COMPARISON_LESS);
  auto no_depth_test_descriptor = depth_state_less_->descriptor();
  no_depth_test_descriptor._depth_test = false;
  depth_state_disable_ = device_->create_depth_stencil_state(no_depth_test_descriptor);

  change_point_size_in_shader_state_ = device_->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, false, false, 0.0, false, false, scm::gl::point_raster_state(true));
  no_backface_culling_rasterizer_state_ = device_->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, false, false, 0.0, false, false);

  filter_linear_ = device_->create_sampler_state(scm::gl::FILTER_ANISOTROPIC, scm::gl::WRAP_CLAMP_TO_EDGE, 16u);  
  filter_nearest_ = device_->create_sampler_state(scm::gl::FILTER_MIN_MAG_LINEAR, scm::gl::WRAP_CLAMP_TO_EDGE);

  float gaussian_buffer[32] = {255, 255, 252, 247, 244, 234, 228, 222, 208, 201,
                              191, 176, 167, 158, 141, 131, 125, 117, 100,  91,
                              87,  71,  65,  58,  48,  42,  39,  32,  28,  25,
                              19, 16};
  scm::gl::texture_region ur(scm::math::vec3ui(0u), scm::math::vec3ui(32, 1, 1));
  gaussian_texture_ = device_->create_texture_2d(scm::math::vec2ui(32,1), scm::gl::FORMAT_R_32F, 1, 1, 1);
  context_->update_sub_texture(gaussian_texture_, ur, 0u, scm::gl::FORMAT_R_32F, gaussian_buffer);

  auto root_bb = database->get_model(0)->get_bvh()->get_bounding_boxes()[0];
  auto root_bb_min = scm::math::mat4f(model_transformations_[0]) * root_bb.min_vertex();
  auto root_bb_max = scm::math::mat4f(model_transformations_[0]) * root_bb.max_vertex();
  scm::math::vec3f center = (root_bb_min + root_bb_max) / 2.f;

  camera_ = new lamure::ren::camera(0, 
    scm::math::make_look_at_matrix(center+scm::math::vec3f(0.f, 0.1f, -0.01f), center, scm::math::vec3f(0.f, 1.f, 0.f)), 
    scm::math::length(root_bb_max-root_bb_min), false, false);
  camera_->set_projection_matrix(settings_.fov_, float(settings_.width_)/float(settings_.height_),  settings_.near_plane_, settings_.far_plane_);
  camera_->set_dolly_sens_(settings_.travel_speed_);

  screen_quad_.reset(new scm::gl::quad_geometry(device_, scm::math::vec2f(-1.0f, -1.0f), scm::math::vec2f(1.0f, 1.0f)));
  
  gui_.ortho_matrix_ = scm::math::make_ortho_matrix(0.0f, static_cast<float>(settings_.width_),
      0.0f, static_cast<float>(settings_.height_), -1.0f, 1.0f);


  try {
    scm::gl::font_face_ptr output_font(new scm::gl::font_face(device_, std::string(LAMURE_FONTS_DIR) + "/Ubuntu.ttf", 20, 0, scm::gl::font_face::smooth_lcd));
    text_renderer_ = scm::make_shared<scm::gl::text_renderer>(device_);
    renderable_text_ = scm::make_shared<scm::gl::text>(device_, output_font, scm::gl::font_face::style_regular, "sick, sad world...");

    text_renderer_->projection_matrix(gui_.ortho_matrix_);
      
    renderable_text_->text_color(scm::math::vec4f(1.0f, 1.0f, 0.0f, 1.0f));
    renderable_text_->text_kerning(true);
  }
  catch(const std::exception& e) {
      throw std::runtime_error(e.what());
  }

  glutMainLoop();

  return 0;


}

