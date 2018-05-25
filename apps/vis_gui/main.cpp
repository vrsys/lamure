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
#include <lamure/vt/VTConfig.h>
#include <lamure/vt/ren/CutDatabase.h>
#include <lamure/vt/ren/CutUpdate.h>
#include <lamure/vt/pre/AtlasFile.h>

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

scm::gl::program_ptr vis_quad_shader_;
scm::gl::program_ptr vis_line_shader_;
scm::gl::program_ptr vis_triangle_shader_;
scm::gl::program_ptr vis_vt_shader_;

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

scm::gl::sampler_state_ptr vt_filter_linear_;
scm::gl::sampler_state_ptr vt_filter_nearest_;

scm::gl::texture_2d_ptr bg_texture_;

struct resource {
  uint64_t num_primitives_ {0};
  scm::gl::buffer_ptr buffer_;
  scm::gl::vertex_array_ptr array_;
};

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

struct input {
  float trackball_x_ = 0.f;
  float trackball_y_ = 0.f;
  scm::math::vec2i mouse_;
  scm::math::vec2i prev_mouse_;
  bool brush_mode_ = 0;
  bool brush_clear_ = 0;
  bool gui_lock_ = false;
  lamure::ren::camera::mouse_state mouse_state_;
};

input input_;

struct gui {
  scm::math::mat4f ortho_matrix_;
};

gui gui_;

struct xyz {
  scm::math::vec3f pos_;
  uint8_t r_;
  uint8_t g_;
  uint8_t b_;
  uint8_t a_;
  float rad_;
  scm::math::vec3f nml_;
};

struct triangle {
    scm::math::vec3f pos_;
    scm::math::vec2f uv_;
};

struct selection {
  int32_t selected_model_ = -1;
  std::vector<xyz> brush_;
  std::set<uint32_t> selected_views_;
};

selection selection_;

struct provenance {
  uint32_t num_views_ {0};
};

std::map<uint32_t, provenance> provenance_;

struct settings {
  int32_t width_ {1920};
  int32_t height_ {1080};
  int32_t frame_div_ {1};
  int32_t vram_ {2048};
  int32_t ram_ {4096};
  int32_t upload_ {32};
  bool provenance_ {0};
  float near_plane_ {0.001f};
  float far_plane_ {1000.0f};
  float fov_ {30.0f};
  bool splatting_ {1};
  bool gamma_correction_ {1};
  int32_t gui_ {1};
  int32_t travel_ {2};
  float travel_speed_ {20.5f};
  bool lod_update_ {1};
  bool use_pvs_ {1};
  bool pvs_culling_ {0};
  float lod_point_scale_ {1.0f};
  float aux_point_size_ {1.0f};
  float aux_point_scale_ {1.0f};
  float aux_focal_length_ {1.0f};
  int32_t vis_ {0};
  int32_t show_normals_ {0};
  bool show_accuracy_ {0};
  bool show_radius_deviation_ {0};
  bool show_output_sensitivity_ {0};
  bool show_sparse_ {0};
  bool show_views_ {0};
  bool show_octrees_ {0};
  int32_t channel_ {0};
  float lod_error_ {LAMURE_DEFAULT_THRESHOLD};
  bool enable_lighting_ {1};
  bool use_material_color_ {0};
  scm::math::vec3f material_diffuse_ {0.6f, 0.6f, 0.6f};
  scm::math::vec4f material_specular_ {0.4f, 0.4f, 0.4f, 1000.0f};
  scm::math::vec3f ambient_light_color_ {0.1f, 0.1f, 0.1f};
  scm::math::vec4f point_light_color_ {1.0f, 1.0f, 1.0f, 1.2f};
  bool heatmap_ {0};
  float heatmap_min_ {0.0f};
  float heatmap_max_ {0.05f};
  scm::math::vec3f background_color_ {LAMURE_DEFAULT_COLOR_R, LAMURE_DEFAULT_COLOR_G, LAMURE_DEFAULT_COLOR_B};
  scm::math::vec3f heatmap_color_min_ {68.0f/255.0f, 0.0f, 84.0f/255.0f};
  scm::math::vec3f heatmap_color_max_ {251.f/255.f, 231.f/255.f, 35.f/255.f};
  std::string atlas_file_ {""};
  std::string json_ {""};
  std::string pvs_ {""};
  std::string background_image_ {""};
  int32_t use_view_tf_ {0};
  scm::math::mat4d view_tf_ {scm::math::mat4d::identity()};
  std::vector<std::string> models_;
  std::map<uint32_t, scm::math::mat4d> transforms_;
  std::map<uint32_t, std::shared_ptr<lamure::prov::octree>> octrees_;
  std::map<uint32_t, std::string> aux_;

};

settings settings_;

struct vt_info {
    uint32_t texture_id_;
    uint16_t view_id_;
    uint16_t context_id_;
    uint64_t cut_id_;
    vt::CutUpdate *cut_update_;

    std::vector<scm::gl::texture_2d_ptr> index_texture_hierarchy_;
    scm::gl::texture_2d_ptr physical_texture_;

    scm::math::vec2ui physical_texture_size_;
    scm::math::vec2ui physical_texture_tile_size_;
    size_t size_feedback_;

    int32_t  *feedback_lod_cpu_buffer_;
    uint32_t *feedback_count_cpu_buffer_;

    scm::gl::buffer_ptr feedback_lod_storage_;
    scm::gl::buffer_ptr feedback_count_storage_;

    int toggle_visualization_;
    bool enable_hierarchy_;
};

vt_info vt_;


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

          settings.models_.push_back(model);
          settings.transforms_[model_id] = scm::math::mat4d::identity();
          settings.aux_[model_id] = "";
          ++model_id;

        }
        else {

          std::string key = line.substr(0, colon);

          if (key[0] == '@') {
            auto ws = line.find_first_of(' ');
            uint32_t address = atoi(strip_whitespace(line.substr(1, ws-1)).c_str());
            key = strip_whitespace(line.substr(ws+1, colon-(ws+1)));
            std::string value = strip_whitespace(line.substr(colon+1));

            if (key == "tf") {
              settings.transforms_[address] = load_matrix(value);
              std::cout << "found transform for model id " << address << std::endl;
            }
            else if (key == "aux") {
              settings.aux_[address] = value;
              std::cout << "found aux data for model id " << address << std::endl;
            }
            else {
              std::cout << "unrecognized key: " << key << std::endl;
              exit(-1);
            }
            continue;
          }

          key = strip_whitespace(key);
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
            settings.splatting_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "gamma_correction") {
            settings.gamma_correction_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "gui") {
            settings.gui_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "speed") {
            settings.travel_speed_ = std::min(std::max(atof(value.c_str()), 0.0), 400.0);
          }
          else if (key == "pvs_culling") {
            settings.pvs_culling_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "use_pvs") {
            settings.use_pvs_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "lod_point_scale") {
            settings.lod_point_scale_ = std::min(std::max(atof(value.c_str()), 0.0), 10.0);
          }
          else if (key == "aux_point_size") {
            settings.aux_point_size_ = std::min(std::max(atof(value.c_str()), 0.001), 1.0);
          }
          else if (key == "aux_focal_length") {
            settings.aux_focal_length_ = std::min(std::max(atof(value.c_str()), 0.001), 10.0);
          }
          else if (key == "lod_error") {
            settings.lod_error_ = std::min(std::max(atof(value.c_str()), 0.0), 10.0);
          }
          else if (key == "provenance") {
            settings.provenance_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "show_normals") {
            settings.show_normals_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "show_accuracy") {
            settings.show_accuracy_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "show_radius_deviation") {
            settings.show_radius_deviation_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "show_output_sensitivity") {
            settings.show_output_sensitivity_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "show_sparse") {
            settings.show_sparse_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "show_views") {
            settings.show_views_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "show_octrees") {
            settings.show_octrees_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "channel") {
            settings.channel_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "enable_lighting") {
            settings.enable_lighting_ = (bool)std::min(std::max(atoi(value.c_str()), 0), 1);
          }
          else if (key == "use_material_color") {
            settings.use_material_color_ = (bool)std::min(std::max(atoi(value.c_str()), 0), 1);
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
            settings.heatmap_ = (bool)std::max(atoi(value.c_str()), 0);
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
          else if (key == "atlas_file") {
            settings.atlas_file_ = value;
          }
          else if (key == "json") {
            settings.json_ = value;
          }
          else if (key == "pvs") {
            settings.pvs_ = value;
          }
          else if (key == "background_image") {
            settings.background_image_ = value;
          }
          else if (key == "use_view_tf") {
            settings.use_view_tf_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "view_tf") {
            settings.view_tf_ = load_matrix(value);
          }
          else {
            std::cout << "unrecognized key: " << key << std::endl;
            exit(-1);
          }

          //std::cout << key << " : " << value << std::endl;
        }

      }
    }
    vis_file.close();
  }

  //assertions
  if (settings.provenance_ != 0) {
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

scm::gl::data_format get_tex_format() {
    switch (vt::VTConfig::get_instance().get_format_texture()) {
        case vt::VTConfig::R8:
            return scm::gl::FORMAT_R_8;
        case vt::VTConfig::RGB8:
            return scm::gl::FORMAT_RGB_8;
        case vt::VTConfig::RGBA8:
        default:
            return scm::gl::FORMAT_RGBA_8;
    }
}

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


void apply_vt_cut_update() {
  auto *cut_db = &vt::CutDatabase::get_instance();

  for (vt::cut_map_entry_type cut_entry : (*cut_db->get_cut_map())) {
    vt::Cut *cut = cut_db->start_reading_cut(cut_entry.first);

    if (!cut->is_drawn()) {
      cut_db->stop_reading_cut(cut_entry.first);
      continue;
    }

    std::set<uint16_t> updated_levels;

    for (auto position_slot_updated : cut->get_front()->get_mem_slots_updated()) {
      const vt::mem_slot_type *mem_slot_updated = cut_db->read_mem_slot_at(position_slot_updated.second);

      if (mem_slot_updated == nullptr || !mem_slot_updated->updated
          || !mem_slot_updated->locked || mem_slot_updated->pointer == nullptr) {
        if (mem_slot_updated == nullptr) {
          std::cerr << "Mem slot at " << position_slot_updated.second << " is null" << std::endl;
        } else {
          std::cerr << "Mem slot at " << position_slot_updated.second << std::endl;
          std::cerr << "Mem slot #" << mem_slot_updated->position << std::endl;
          std::cerr << "Tile id: " << mem_slot_updated->tile_id << std::endl;
          std::cerr << "Locked: " << mem_slot_updated->locked << std::endl;
          std::cerr << "Updated: " << mem_slot_updated->updated << std::endl;
          std::cerr << "Pointer valid: " << (mem_slot_updated->pointer != nullptr) << std::endl;
        }

        throw std::runtime_error("updated mem slot inconsistency");
      }

      updated_levels.insert(vt::QuadTree::get_depth_of_node(mem_slot_updated->tile_id));

      // update_physical_texture_blockwise
      size_t slots_per_texture = vt::VTConfig::get_instance().get_phys_tex_tile_width() *
                                 vt::VTConfig::get_instance().get_phys_tex_tile_width();
      size_t layer = mem_slot_updated->position / slots_per_texture;
      size_t rel_slot_position = mem_slot_updated->position - layer * slots_per_texture;

      size_t x_tile = rel_slot_position % vt::VTConfig::get_instance().get_phys_tex_tile_width();
      size_t y_tile = rel_slot_position / vt::VTConfig::get_instance().get_phys_tex_tile_width();

      scm::math::vec3ui origin = scm::math::vec3ui(
              (uint32_t) x_tile * vt::VTConfig::get_instance().get_size_tile(),
              (uint32_t) y_tile * vt::VTConfig::get_instance().get_size_tile(), (uint32_t) layer);
      scm::math::vec3ui dimensions = scm::math::vec3ui(vt::VTConfig::get_instance().get_size_tile(),
                                                       vt::VTConfig::get_instance().get_size_tile(), 1);

      context_->update_sub_texture(vt_.physical_texture_, scm::gl::texture_region(origin, dimensions), 0,
                                   get_tex_format(), mem_slot_updated->pointer);
    }


    for (auto position_slot_cleared : cut->get_front()->get_mem_slots_cleared()) {
      const vt::mem_slot_type *mem_slot_cleared = cut_db->read_mem_slot_at(position_slot_cleared.second);

      if (mem_slot_cleared == nullptr) {
        std::cerr << "Mem slot at " << position_slot_cleared.second << " is null" << std::endl;
      }

      updated_levels.insert(vt::QuadTree::get_depth_of_node(position_slot_cleared.first));
    }

    // update_index_texture
    for (uint16_t updated_level : updated_levels) {
      uint32_t size_index_texture = (uint32_t) vt::QuadTree::get_tiles_per_row(updated_level);

      scm::math::vec3ui origin = scm::math::vec3ui(0, 0, 0);
      scm::math::vec3ui dimensions = scm::math::vec3ui(size_index_texture, size_index_texture, 1);

      context_->update_sub_texture(vt_.index_texture_hierarchy_.at(updated_level),
                                   scm::gl::texture_region(origin, dimensions), 0, scm::gl::FORMAT_RGBA_8UI,
                                   cut->get_front()->get_index(updated_level));

    }

    cut_db->stop_reading_cut(cut_entry.first);
  }

  context_->sync();
}

void collect_vt_feedback() {
  int32_t *feedback_lod = (int32_t *) context_->map_buffer(vt_.feedback_lod_storage_, scm::gl::ACCESS_READ_ONLY);
  memcpy(vt_.feedback_lod_cpu_buffer_, feedback_lod, vt_.size_feedback_ * size_of_format(scm::gl::FORMAT_R_32I));
  context_->sync();

  context_->unmap_buffer(vt_.feedback_lod_storage_);
  context_->clear_buffer_data(vt_.feedback_lod_storage_, scm::gl::FORMAT_R_32I, nullptr);

  uint32_t *feedback_count = (uint32_t *) context_->map_buffer(vt_.feedback_count_storage_,
                                                               scm::gl::ACCESS_READ_ONLY);
  memcpy(vt_.feedback_count_cpu_buffer_, feedback_count, vt_.size_feedback_ * size_of_format(scm::gl::FORMAT_R_32UI));
  context_->sync();

  vt_.cut_update_->feedback(vt_.feedback_lod_cpu_buffer_, vt_.feedback_count_cpu_buffer_);

  context_->unmap_buffer(vt_.feedback_count_storage_);
  context_->clear_buffer_data(vt_.feedback_count_storage_, scm::gl::FORMAT_R_32UI, nullptr);
}

void draw_resources() {

  if (sparse_resources_.size() > 0) { 
    if ((settings_.show_sparse_ || settings_.show_views_) && sparse_resources_.size() > 0) {

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
    if (settings_.show_views_ && !settings_.atlas_file_.empty()) {
      context_->bind_program(vis_vt_shader_);

      uint64_t color_cut_id =
              (((uint64_t) vt_.texture_id_) << 32) | ((uint64_t) vt_.view_id_ << 16) | ((uint64_t) vt_.context_id_);
      uint32_t max_depth_level_color =
              (*vt::CutDatabase::get_instance().get_cut_map())[color_cut_id]->get_atlas()->getDepth() - 1;

      scm::math::mat4f view_matrix       = camera_->get_view_matrix();
      scm::math::mat4f projection_matrix = scm::math::mat4f(camera_->get_projection_matrix());

      vis_vt_shader_->uniform("model_view_matrix", view_matrix);
      vis_vt_shader_->uniform("projection_matrix", projection_matrix);

      vis_vt_shader_->uniform("physical_texture_dim", vt_.physical_texture_size_);
      vis_vt_shader_->uniform("max_level", max_depth_level_color);
      vis_vt_shader_->uniform("tile_size", scm::math::vec2((uint32_t) vt::VTConfig::get_instance().get_size_tile()));
      vis_vt_shader_->uniform("tile_padding", scm::math::vec2((uint32_t) vt::VTConfig::get_instance().get_size_padding()));

      vis_vt_shader_->uniform("enable_hierarchy", vt_.enable_hierarchy_);
      vis_vt_shader_->uniform("toggle_visualization", vt_.toggle_visualization_);

      for (uint32_t i = 0; i < vt_.index_texture_hierarchy_.size(); ++i) {
        std::string texture_string = "hierarchical_idx_textures";
        vis_vt_shader_->uniform(texture_string, i, int((i)));
      }

      vis_vt_shader_->uniform("physical_texture_array", 17);

      context_->set_viewport(
              scm::gl::viewport(scm::math::vec2ui(0, 0), 1 * scm::math::vec2ui(render_width_, render_height_)));

      context_->set_depth_stencil_state(depth_state_less_);
      context_->set_rasterizer_state(no_backface_culling_rasterizer_state_);
      context_->set_blend_state(color_no_blending_state_);

      context_->sync();

      apply_vt_cut_update();

      for (uint16_t i = 0; i < vt_.index_texture_hierarchy_.size(); ++i) {
        context_->bind_texture(vt_.index_texture_hierarchy_.at(i), vt_filter_nearest_, i);
      }

      context_->bind_texture(vt_.physical_texture_, vt_filter_linear_, 17);

      context_->bind_storage_buffer(vt_.feedback_lod_storage_, 0);
      context_->bind_storage_buffer(vt_.feedback_count_storage_, 1);

      context_->apply();

      for (int32_t model_id = 0; model_id < num_models_; ++model_id) {
        if (selection_.selected_model_ != -1) {
          model_id = selection_.selected_model_;
        }

        auto t_res = image_plane_resources_[model_id];

        if (t_res.num_primitives_ > 0) {
          context_->bind_vertex_array(t_res.array_);
          context_->apply();
          if (selection_.selected_views_.empty()) {
            context_->draw_arrays(scm::gl::PRIMITIVE_TRIANGLE_LIST, 0, t_res.num_primitives_);
          }
          else {
            for (const auto view : selection_.selected_views_) {
              context_->draw_arrays(scm::gl::PRIMITIVE_TRIANGLE_LIST, view * 16, 16);
            }
          }
        }

        if (selection_.selected_model_ != -1) {
          break;
        }
      }
      context_->sync();

      collect_vt_feedback();

    }

    if (settings_.show_views_ || settings_.show_octrees_) {
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
                context_->draw_arrays(scm::gl::PRIMITIVE_LINE_LIST, view*16, 16);
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

  while(parse_prefix(relative_path, "../")) {
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

    auto selected_pass2_shading_program = vis_xyz_pass2_shader_;

    if(settings_.enable_lighting_) {
      selected_pass2_shading_program = vis_xyz_pass2_lighting_shader_;
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

    auto selected_pass3_shading_program = vis_xyz_pass3_shader_;

    if(settings_.enable_lighting_) {
      selected_pass3_shading_program = vis_xyz_pass3_lighting_shader_;
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

    auto selected_single_pass_shading_program = vis_xyz_shader_;

    if(settings_.enable_lighting_) {
      selected_single_pass_shading_program = vis_xyz_lighting_shader_;
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

    context_->bind_program(vis_xyz_shader_);
    draw_brush(vis_xyz_shader_);
    draw_resources();


  }


  //PASS 4: fullscreen quad
  
  context_->clear_default_depth_stencil_buffer();
  context_->clear_default_color_buffer();
  context_->set_default_frame_buffer();
  context_->set_depth_stencil_state(depth_state_disable_);
  
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

  gui_.ortho_matrix_ = 
    scm::math::make_ortho_matrix(0.0f, static_cast<float>(settings_.width_),
    0.0f, static_cast<float>(settings_.height_), -1.0f, 1.0f);

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

  input_.prev_mouse_ = input_.mouse_;
  input_.mouse_ = scm::math::vec2i(x, y);
  
  if (!input_.brush_mode_ && !input_.gui_lock_) {
    camera_->update_trackball(x, y, settings_.width_, settings_.height_, input_.mouse_state_);
  }
  else {
    brush();
  }
}

float get_atlas_scale_factor() {
  auto atlas = new vt::pre::AtlasFile(settings_.atlas_file_.c_str());
  uint64_t image_width    = atlas->getImageWidth();
  uint64_t image_height   = atlas->getImageHeight();

  // tile's width and height without padding
  uint64_t tile_inner_width  = atlas->getInnerTileWidth();
  uint64_t tile_inner_height = atlas->getInnerTileHeight();

  // Quadtree depth counter, ranges from 0 to depth-1
  uint64_t depth = atlas->getDepth();

  double factor_u  = (double) image_width  / (tile_inner_width  * std::pow(2, depth-1));
  double factor_v  = (double) image_height / (tile_inner_height * std::pow(2, depth-1));

  return std::max(factor_u, factor_v);
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

        std::vector<triangle> tris_to_upload;
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
          float factor = get_atlas_scale_factor();

          // positions in vt atlas space coordinate system
          float tile_height  = (float) atlas_tile.width_  / atlas_width  * factor;
          float tile_width   = (float) atlas_tile.width_  / atlas_height * factor;

          float tile_pos_x   = (float) atlas_tile.x_ / atlas_height * factor;
          float tile_pos_y   = (float) atlas_tile.y_ / atlas_tile.height_ * tile_height + (1 - factor);


          triangle p1;
          p1.pos_ = view.transform_ * scm::math::vec3f(-img_w_half, img_h_half, -focal_length);
          p1.uv_  = scm::math::vec2f(tile_pos_x + tile_width, tile_pos_y);

          triangle p2;
          p2.pos_ = view.transform_ * scm::math::vec3f(img_w_half, img_h_half, -focal_length);
          p2.uv_  = scm::math::vec2f(tile_pos_x, tile_pos_y);

          triangle p3;
          p3.pos_ = view.transform_ * scm::math::vec3f(-img_w_half, -img_h_half, -focal_length);
          p3.uv_  = scm::math::vec2f(tile_pos_x + tile_width, tile_pos_y + tile_height);

          triangle p4;
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
                                                 (sizeof(triangle)) * tris_to_upload.size(),
                                                 &tris_to_upload[0]);

        tri_res.array_ = device_->create_vertex_array(scm::gl::vertex_format
                                                              (0, 0, scm::gl::TYPE_VEC3F, sizeof(triangle))
                                                              (0, 1, scm::gl::TYPE_VEC2F, sizeof(triangle)),
                                                      boost::assign::list_of(tri_res.buffer_));


        tri_res.num_primitives_ = tris_to_upload.size();

        image_plane_resources_[model_id] = tri_res;
      }

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



struct Window {
  Window() {
    _mouse_button_state = MouseButtonState::IDLE;
  }

  unsigned int _width;
  unsigned int _height;

  GLFWwindow *_glfw_window;

  enum MouseButtonState {
    LEFT = 0,
    WHEEL = 1,
    RIGHT = 2,
    IDLE = 3
  };

  MouseButtonState _mouse_button_state;
};

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
      
      ImGui_ImplGlfwGL3_MouseButtonCallback(glfw_window, button, action, mods);
    }

    static void on_window_move_cursor(GLFWwindow *glfw_window, double x, double y) {
      Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
      
      input_.prev_mouse_ = input_.mouse_;
      input_.mouse_ = scm::math::vec2i(x, y);
  
      if (!input_.brush_mode_ && !input_.gui_lock_) {
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

      if (!input_.brush_mode_ && !input_.gui_lock_) {
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

  vt_filter_linear_  = device_->create_sampler_state(scm::gl::FILTER_MIN_MAG_LINEAR , scm::gl::WRAP_CLAMP_TO_EDGE);
  vt_filter_nearest_ = device_->create_sampler_state(scm::gl::FILTER_MIN_MAG_NEAREST, scm::gl::WRAP_CLAMP_TO_EDGE);
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

  gui_.ortho_matrix_ = scm::math::make_ortho_matrix(0.0f, static_cast<float>(settings_.width_),
                                                    0.0f, static_cast<float>(settings_.height_), -1.0f, 1.0f);
}


void init_vt_database() {
  vt::VTConfig::CONFIG_PATH = settings_.atlas_file_.substr(0, settings_.atlas_file_.size() - 5) + "ini";

  vt::VTConfig::get_instance().define_size_physical_texture(128, 8192);
  vt_.texture_id_ = vt::CutDatabase::get_instance().register_dataset(settings_.atlas_file_);
  vt_.context_id_ = vt::CutDatabase::get_instance().register_context();
  vt_.view_id_    = vt::CutDatabase::get_instance().register_view();
  vt_.cut_id_     = vt::CutDatabase::get_instance().register_cut(vt_.texture_id_, vt_.view_id_, vt_.context_id_);
  vt_.cut_update_ = &vt::CutUpdate::get_instance();
  vt_.cut_update_->start();
}


void init_vt_system() {
    vt_.enable_hierarchy_     = true;
    vt_.toggle_visualization_ = 0;

    // add_data
    uint16_t depth = (uint16_t) ((*vt::CutDatabase::get_instance().get_cut_map())[vt_.cut_id_]->get_atlas()->getDepth());
    uint16_t level = 0;

    while (level < depth) {
        uint32_t size_index_texture = (uint32_t) vt::QuadTree::get_tiles_per_row(level);

        auto index_texture_level_ptr = device_->create_texture_2d(
                scm::math::vec2ui(size_index_texture, size_index_texture), scm::gl::FORMAT_RGBA_8UI);

        device_->main_context()->clear_image_data(index_texture_level_ptr, 0, scm::gl::FORMAT_RGBA_8UI, 0);
        vt_.index_texture_hierarchy_.emplace_back(index_texture_level_ptr);

        level++;
    }

    // add_context
    context_ = device_->main_context();
    vt_.physical_texture_size_ = scm::math::vec2ui(vt::VTConfig::get_instance().get_phys_tex_tile_width(),
                                                   vt::VTConfig::get_instance().get_phys_tex_tile_width());

    auto physical_texture_size = scm::math::vec2ui(vt::VTConfig::get_instance().get_phys_tex_px_width(),
                                                   vt::VTConfig::get_instance().get_phys_tex_px_width());

    vt_.physical_texture_ = device_->create_texture_2d(physical_texture_size, get_tex_format(), 1,
                                                       vt::VTConfig::get_instance().get_phys_tex_layers() + 1);

    vt_.size_feedback_ = vt::VTConfig::get_instance().get_phys_tex_tile_width() *
                         vt::VTConfig::get_instance().get_phys_tex_tile_width() *
                         vt::VTConfig::get_instance().get_phys_tex_layers();

    vt_.feedback_lod_storage_   = device_->create_buffer(scm::gl::BIND_STORAGE_BUFFER, scm::gl::USAGE_STREAM_COPY,
                                                         vt_.size_feedback_ * size_of_format(scm::gl::FORMAT_R_32I));
    vt_.feedback_count_storage_ = device_->create_buffer(scm::gl::BIND_STORAGE_BUFFER, scm::gl::USAGE_STREAM_COPY,
                                                         vt_.size_feedback_ * size_of_format(scm::gl::FORMAT_R_32UI));

    vt_.feedback_lod_cpu_buffer_   = new int32_t[vt_.size_feedback_];
    vt_.feedback_count_cpu_buffer_ = new uint32_t[vt_.size_feedback_];

    for (size_t i = 0; i < vt_.size_feedback_; ++i) {
        vt_.feedback_lod_cpu_buffer_[i] = 0;
        vt_.feedback_count_cpu_buffer_[i] = 0;
    }
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

  try
  {
    std::string vis_quad_vs_source;
    std::string vis_quad_fs_source;
    std::string vis_line_vs_source;
    std::string vis_line_fs_source;
    
    std::string vis_triangle_vs_source;
    std::string vis_triangle_fs_source;

    std::string vis_vt_vs_source;
    std::string vis_vt_fs_source;

    std::string vis_xyz_vs_source;
    std::string vis_xyz_gs_source;
    std::string vis_xyz_fs_source;
    
    std::string vis_xyz_pass1_vs_source;
    std::string vis_xyz_pass1_gs_source;
    std::string vis_xyz_pass1_fs_source;
    std::string vis_xyz_pass2_vs_source;
    std::string vis_xyz_pass2_gs_source;
    std::string vis_xyz_pass2_fs_source;
    std::string vis_xyz_pass3_vs_source;
    std::string vis_xyz_pass3_fs_source;


    std::string vis_xyz_qz_vs_source;
    std::string vis_xyz_qz_pass1_vs_source;
    std::string vis_xyz_qz_pass2_vs_source;

    /* parsed with optional lighting code */
    std::string vis_xyz_vs_lighting_source;
    std::string vis_xyz_gs_lighting_source;
    std::string vis_xyz_fs_lighting_source;
    std::string vis_xyz_pass2_vs_lighting_source;
    std::string vis_xyz_pass2_gs_lighting_source;
    std::string vis_xyz_pass2_fs_lighting_source;
    std::string vis_xyz_pass3_vs_lighting_source;
    std::string vis_xyz_pass3_fs_lighting_source;

    std::string shader_root_path = LAMURE_SHADERS_DIR;

    if (!read_shader(shader_root_path + "/vis/vis_quad.glslv", vis_quad_vs_source)
      || !read_shader(shader_root_path + "/vis/vis_quad.glslf", vis_quad_fs_source)
      || !read_shader(shader_root_path + "/vis/vis_line.glslv", vis_line_vs_source)
      || !read_shader(shader_root_path + "/vis/vis_line.glslf", vis_line_fs_source)
      || !read_shader(shader_root_path + "/vis/vis_triangle.glslv", vis_triangle_vs_source)
      || !read_shader(shader_root_path + "/vis/vis_triangle.glslf", vis_triangle_fs_source)

      || !read_shader(shader_root_path + "/vt/virtual_texturing.glslv", vis_vt_vs_source)
      || !read_shader(shader_root_path + "/vt/virtual_texturing_hierarchical.glslf", vis_vt_fs_source)

      || !read_shader(shader_root_path + "/vis/vis_xyz.glslv", vis_xyz_vs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz.glslg", vis_xyz_gs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz.glslf", vis_xyz_fs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz_pass1.glslv", vis_xyz_pass1_vs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz_pass1.glslg", vis_xyz_pass1_gs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz_pass1.glslf", vis_xyz_pass1_fs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz_pass2.glslv", vis_xyz_pass2_vs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz_pass2.glslg", vis_xyz_pass2_gs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz_pass2.glslf", vis_xyz_pass2_fs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz_pass3.glslv", vis_xyz_pass3_vs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz_pass3.glslf", vis_xyz_pass3_fs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz_qz.glslv", vis_xyz_qz_vs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz_qz_pass1.glslv", vis_xyz_qz_pass1_vs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz_qz_pass2.glslv", vis_xyz_qz_pass2_vs_source)

      || !read_shader(shader_root_path + "/vis/vis_xyz.glslv", vis_xyz_vs_lighting_source, true)
      || !read_shader(shader_root_path + "/vis/vis_xyz.glslg", vis_xyz_gs_lighting_source, true)
      || !read_shader(shader_root_path + "/vis/vis_xyz.glslf", vis_xyz_fs_lighting_source, true)
      || !read_shader(shader_root_path + "/vis/vis_xyz_pass2.glslv", vis_xyz_pass2_vs_lighting_source, true)
      || !read_shader(shader_root_path + "/vis/vis_xyz_pass2.glslg", vis_xyz_pass2_gs_lighting_source, true)
      || !read_shader(shader_root_path + "/vis/vis_xyz_pass2.glslf", vis_xyz_pass2_fs_lighting_source, true)
      || !read_shader(shader_root_path + "/vis/vis_xyz_pass3.glslv", vis_xyz_pass3_vs_lighting_source, true)
      || !read_shader(shader_root_path + "/vis/vis_xyz_pass3.glslf", vis_xyz_pass3_fs_lighting_source, true)
      ) {
      std::cout << "error reading shader files" << std::endl;
      exit(1);
    }

    vis_quad_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_quad_vs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_quad_fs_source)));
    if (!vis_quad_shader_) {
      std::cout << "error creating shader vis_quad_shader_ program" << std::endl;
      exit(1);
    }

    vis_line_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_line_vs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_line_fs_source)));
    if (!vis_line_shader_) {
      std::cout << "error creating shader vis_line_shader_ program" << std::endl;
      exit(1);
    }

    vis_triangle_shader_ = device_->create_program(
            boost::assign::list_of
                    (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_triangle_vs_source))
                    (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_triangle_fs_source)));
    if (!vis_triangle_shader_) {
      std::cout << "error creating shader vis_triangle_shader_ program" << std::endl;
      std::exit(1);
    }

    vis_vt_shader_ = device_->create_program(
            boost::assign::list_of
                    (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_vt_vs_source))
                    (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_vt_fs_source)));
    if (!vis_vt_shader_) {
      std::cout << "error creating shader vis_vt_shader_ program" << std::endl;
      std::exit(1);
    }

    vis_xyz_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_vs_source))
        (device_->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, vis_xyz_gs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_fs_source)));
    if (!vis_xyz_shader_) {
      //scm::err() << scm::log::error << scm::log::end;
      std::cout << "error creating shader vis_xyz_shader_ program" << std::endl;
      exit(1);
    }

    vis_xyz_pass1_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_pass1_vs_source))
        (device_->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, vis_xyz_pass1_gs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_pass1_fs_source)));
    if (!vis_xyz_pass1_shader_) {
      std::cout << "error creating vis_xyz_pass1_shader_ program" << std::endl;
      exit(1);
    }

    vis_xyz_pass2_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_pass2_vs_source))
        (device_->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, vis_xyz_pass2_gs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_pass2_fs_source)));
    if (!vis_xyz_pass2_shader_) {
      std::cout << "error creating vis_xyz_pass2_shader_ program" << std::endl;
      exit(1);
    }

    vis_xyz_pass3_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_pass3_vs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_pass3_fs_source)));
    if (!vis_xyz_pass3_shader_) {
      std::cout << "error creating vis_xyz_pass3_shader_ program" << std::endl;
      exit(1);
    }

    vis_xyz_lighting_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_vs_lighting_source))
        (device_->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, vis_xyz_gs_lighting_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_fs_lighting_source)));
    if (!vis_xyz_lighting_shader_) {
      std::cout << "error creating vis_xyz_lighting_shader_ program" << std::endl;
      exit(1);
    }

    vis_xyz_pass2_lighting_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_pass2_vs_lighting_source))
        (device_->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, vis_xyz_pass2_gs_lighting_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_pass2_fs_lighting_source)));
    if (!vis_xyz_pass2_lighting_shader_) {
      std::cout << "error creating vis_xyz_pass2_lighting_shader_ program" << std::endl;
      exit(1);
    }

    vis_xyz_pass3_lighting_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_pass3_vs_lighting_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_pass3_fs_lighting_source)));
    if (!vis_xyz_pass3_lighting_shader_) {
      std::cout << "error creating vis_xyz_pass3_lighting_shader_ program" << std::endl;
      exit(1);
    }

/*
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
*/
  }
  catch (std::exception& e)
  {
      std::cout << e.what() << std::endl;
  }


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


  create_aux_resources();
  create_framebuffers();

  init_render_states();

  if (!settings_.atlas_file_.empty()) {
    init_vt_database();
    init_vt_system();
  }

  init_camera();


  if (settings_.background_image_ != "") {
    //std::cout << "background image: " << settings_.background_image_ << std::endl;
    scm::gl::texture_loader tl;
    bg_texture_ = tl.load_texture_2d(*device_, settings_.background_image_, true, false);
  }



}

void gui_view_settings(){
    bool view_settings_active = true;

    ImGui::SetNextWindowPos(ImVec2(20, 390));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 180.0f));
    ImGui::Begin("View Settings", &view_settings_active, ImGuiWindowFlags_MenuBar);
    ImGui::SliderFloat("Near Plane", &settings_.near_plane_, 0, 1.0f, "%.4f", 4.0f);
    ImGui::SliderFloat("Far Plane", &settings_.far_plane_, 0, 1000.0f, "%.4f", 4.0f);
    ImGui::SliderFloat("FOV", &settings_.fov_, 18, 60.0f);
    ImGui::SliderFloat("Travel Speed", &settings_.travel_speed_, 0.5f, 300.0f, "%.4f", 4.0f);

    ImGui::End();
}


void gui_lod_settings(){
    bool lod_settings_active = true;

    ImGui::SetNextWindowPos(ImVec2(20, 590));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 210.0f));

    ImGui::Begin("LOD Settings", &lod_settings_active, ImGuiWindowFlags_MenuBar);
    
    ImGui::Checkbox("Lod Update", &settings_.lod_update_);

    ImGui::SliderFloat("LOD Error", &settings_.lod_error_, 1.0f, 10.0f, "%.4f", 2.5f);
    ImGui::SliderFloat("LOD Point Scale", &settings_.lod_point_scale_, 0.1f, 2.0f, "%.4f", 1.0f);
    ImGui::Checkbox("Use PVS", &settings_.use_pvs_);
    
    lamure::pvs::pvs_database::get_instance()->activate(settings_.use_pvs_);

    
    ImGui::Checkbox("PVS Culling", &settings_.pvs_culling_);
    

    ImGui::End();
}

void gui_visual_settings(){
    bool visual_settings_active = true;

    uint32_t num_attributes = 5 + data_provenance_.get_size_in_bytes()/sizeof(float);

    const char* vis_values[] = {
      "Color", "Normals", "Accuracy", 
      "Radius Deviation", "Output Sensitivity", 
      "Provenance 1", "Provenance 2", "Provenance 3", 
      "Provenance 4", "Provenance 5", "Provenance 6", "Provenance 7" };
    static int it = settings_.vis_;

    ImGui::SetNextWindowPos(ImVec2(settings_.width_-520, 20));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 305.0f));
    ImGui::Begin("Visual Settings", &visual_settings_active, ImGuiWindowFlags_MenuBar);
    
    uint32_t num_vis_entries = (5 + data_provenance_.get_size_in_bytes()/sizeof(float));
    ImGui::Combo("Vis", &it, vis_values, num_vis_entries);
    settings_.vis_ = it;

      if(settings_.vis_ > num_vis_entries) {
        settings_.vis_ = 0;
      }
      settings_.show_normals_ = (settings_.vis_ == 1);
      settings_.show_accuracy_ = (settings_.vis_ == 2);
      settings_.show_radius_deviation_ = (settings_.vis_ == 3);
      settings_.show_output_sensitivity_ = (settings_.vis_ == 4);
      if (settings_.vis_ > 4) {
        settings_.channel_ = (settings_.vis_-4);
      }
      else {
        settings_.channel_ = 0;
      }
      

    ImGui::Checkbox("Splatting", &settings_.splatting_);
    ImGui::Checkbox("Gamma Correction", &settings_.gamma_correction_);

    ImGui::Checkbox("Enable Lighting", &settings_.enable_lighting_);
    ImGui::Checkbox("Use Material Color", &settings_.use_material_color_);
    
    static ImVec4 color_mat_diff = ImColor(0.6f, 0.6f, 0.6f, 1.0f);
    ImGui::Text("Material Diffuse");
    ImGui::ColorEdit3("Diffuse", (float*)&color_mat_diff, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoAlpha);
    settings_.material_diffuse_.x = color_mat_diff.x;
    settings_.material_diffuse_.y = color_mat_diff.y;
    settings_.material_diffuse_.z = color_mat_diff.z;
/*
    static ImVec4 color_mat_spec = ImColor(0.4f, 0.4f, 0.4f, 1.0f);
    ImGui::Text("Material Specular");
    ImGui::ColorEdit3("Specular", (float*)&color_mat_spec, ImGuiColorEditFlags_Float);
    settings_.material_specular_.x = color_mat_spec.x;
    settings_.material_specular_.y = color_mat_spec.y;
    settings_.material_specular_.z = color_mat_spec.z;

    static ImVec4 color_ambient_light = ImColor(0.1f, 0.1f, 0.1f, 1.0f);
    ImGui::Text("Ambient Light Color");
    ImGui::ColorEdit3("Ambient", (float*)&color_ambient_light, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoAlpha);
    settings_.ambient_light_color_.x = color_ambient_light.x;
    settings_.ambient_light_color_.y = color_ambient_light.y;
    settings_.ambient_light_color_.z = color_ambient_light.z;

    static ImVec4 color_point_light = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
    ImGui::Text("Point Light Color");
    ImGui::ColorEdit3("Point", (float*)&color_point_light, ImGuiColorEditFlags_Float);
    settings_.point_light_color_.x = color_point_light.x;
    settings_.point_light_color_.y = color_point_light.y;
    settings_.point_light_color_.z = color_point_light.z;
*/
    static ImVec4 background_color = ImColor(0.5f, 0.5f, 0.5f, 1.0f);
    ImGui::Text("Background Color");
    ImGui::ColorEdit3("Background", (float*)&background_color, ImGuiColorEditFlags_Float);
    settings_.background_color_.x = background_color.x;
    settings_.background_color_.y = background_color.y;
    settings_.background_color_.z = background_color.z;

    ImGui::End();
}


void gui_provenance_settings(){
    
    bool provenance_settings_active = true;
    
    ImGui::SetNextWindowPos(ImVec2(settings_.width_-520, 345));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 425.0f));
    ImGui::Begin("Provenance Settings", &provenance_settings_active, ImGuiWindowFlags_MenuBar);

    if (ImGui::SliderFloat("AUX Point Size", &settings_.aux_point_size_, 0.1f, 10.0f, "%.4f", 4.0f)) {
      input_.gui_lock_ = true;
    }
    else input_.gui_lock_ = false;
    ImGui::SliderFloat("AUX Point Scale", &settings_.aux_point_scale_, 0.1f, 2.0f, "%.4f", 4.0f);
    ImGui::SliderFloat("AUX Focal Length", &settings_.aux_focal_length_, 0.1f, 2.0f, "%.4f", 4.0f);


    ImGui::Checkbox("Show Sparse", &settings_.show_sparse_);
    if (settings_.show_sparse_) {
        settings_.enable_lighting_ = false;
        settings_.splatting_ = false;
    }

    ImGui::Checkbox("Show Views", &settings_.show_views_);
    if (settings_.show_views_) {
      settings_.enable_lighting_ = false;
      settings_.splatting_ = false;
    }


    ImGui::Checkbox("Show Octrees", &settings_.show_octrees_);
    if (settings_.show_octrees_) {
        settings_.enable_lighting_ = false;
        settings_.splatting_ = false;
    }

    ImGui::Checkbox("Heatmap", &settings_.heatmap_);
    

    ImGui::InputFloat("Heatmap MIN", &settings_.heatmap_min_);
    ImGui::InputFloat("Heatmap MAX", &settings_.heatmap_max_);

    static ImVec4 color_heatmap_min = ImColor(68.0f/255.0f, 0.0f, 84.0f/255.0f, 1.0f);
    ImGui::Text("Heatmap Color Min");
    ImGui::ColorEdit3("Heatmap Min", (float*)&color_heatmap_min, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoAlpha);
    settings_.heatmap_color_min_.x = color_heatmap_min.x;
    settings_.heatmap_color_min_.y = color_heatmap_min.y;
    settings_.heatmap_color_min_.z = color_heatmap_min.z;

    static ImVec4 color_heatmap_max = ImColor(251.f/255.f, 231.f/255.f, 35.f/255.f, 1.0f);
    ImGui::Text("Heatmap Color Max");
    ImGui::ColorEdit3("Heatmap Max", (float*)&color_heatmap_max, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoAlpha);
    settings_.heatmap_color_max_.x = color_heatmap_max.x;
    settings_.heatmap_color_max_.y = color_heatmap_max.y;
    settings_.heatmap_color_max_.z = color_heatmap_max.z;


    ImGui::End();
}


void gui_status_screen(){
    bool status_screen_active = true;
    static bool view_screen_active = false;
    static bool visual_screen_active = true;
    static bool lod_screen_active = false;
    static bool provenance_screen_active = false;


    static int dataset = 0;

    char* vis_values[num_models_+1] = { };
    for (int i=0; i<num_models_+1; i++) {
        char buffer [32];
        snprintf(buffer, sizeof(buffer), "%s%d", "Dataset ", i);
        if(i==num_models_){
           snprintf(buffer, sizeof(buffer), "%s", "All");
        }
        vis_values[i] = strdup(buffer);
    }

    ImGui::SetNextWindowPos(ImVec2(20, 20));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 350.0f));
    ImGui::Begin("Status Screen", &status_screen_active, ImGuiWindowFlags_MenuBar);
    ImGui::Text("fps %d", (int32_t)fps_);

    double f = (rendered_splats_ / 1000000.0);
    
    std::stringstream stream;
    stream << std::setprecision(2) << f;
    std::string s = stream.str();

    ImGui::Text("# points %s mio.", s.c_str());
    ImGui::Text("# nodes %d", (uint64_t)rendered_nodes_);
    ImGui::Text("# models %d", num_models_);
    
    ImGui::Combo("Dataset", &dataset, vis_values, IM_ARRAYSIZE(vis_values));
  
    if(dataset == (sizeof vis_values / sizeof vis_values[0]) - 1){
      selection_.selected_model_ = -1;
    }else{
      selection_.selected_model_ = dataset;
    }

    ImGui::Checkbox("View Settings", &view_screen_active);
    ImGui::Checkbox("LOD Settings", &lod_screen_active);
    ImGui::Checkbox("Visual Settings", &visual_screen_active);
    if (settings_.provenance_) {
      ImGui::Checkbox("Provenance Settings", &provenance_screen_active);
    }
    ImGui::Checkbox("Brush", &input_.brush_mode_);
    if (ImGui::Checkbox("Clear Brush", &input_.brush_clear_)) {
      selection_.brush_.clear();
      selection_.selected_views_.clear();
      input_.brush_clear_ = false;
    }
    
    if (view_screen_active){
        gui_view_settings();
    }
      
    if (lod_screen_active){
        gui_lod_settings();
    }

    if (visual_screen_active){
        gui_visual_settings();
    }

    if (settings_.provenance_ && provenance_screen_active){
        gui_provenance_settings();
    }

    ImGui::End();
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

  load_settings(vis_file, settings_);

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
          gui_status_screen();
          ImGui::Render();
        }

      }
      glfwSwapBuffers(window->_glfw_window);
    }
  }

  return EXIT_SUCCESS;
}
