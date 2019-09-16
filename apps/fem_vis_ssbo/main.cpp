// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "fem_parser_utils.h"
#include "fem_vis_ssbo_settings.h"


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
#include <scm/gl_util/primitives/geometry.h>
#include <scm/gl_util/primitives/box.h>

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

#include <lamure/ren/ray.h>
#include <lamure/prov/auxi.h>
#include <lamure/prov/octree.h>
#include <lamure/vt/VTConfig.h>
#include <lamure/vt/ren/CutDatabase.h>
#include <lamure/vt/ren/CutUpdate.h>
#include <lamure/vt/pre/AtlasFile.h>

#include <lamure/ren/3rd_party/json.h>

#include <algorithm>
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
#include <sstream>



void refresh_ssbo_data();

struct fem_result_meta_data{
  std::string name;
  float min_absolute_deform;
  float max_absolute_deform;
  scm::math::vec3f min_deformation;
  scm::math::vec3f max_deformation;
};

std::ostream& operator << (std::ostream& o, fem_result_meta_data const& v){
  o << "fem_result_meta_data" << std::endl;
  o << "name: " << v.name << std::endl;
  o << "min_absolute_deform: " << v.min_absolute_deform << std::endl;
  o << "max_absolute_deform: " << v.max_absolute_deform << std::endl;
  o << "min_deformation: " << v.min_deformation << std::endl;
  o << "max_deformation: " << v.max_deformation << std::endl;
  return o;
}

std::vector<fem_result_meta_data> fem_md;

bool rendering_ = false;
int32_t render_width_ = 1280;
int32_t render_height_ = 720;


scm::shared_ptr<scm::gl::text_renderer> text_renderer  =  nullptr;//   scm::make_shared<text_renderer>(device_);
scm::shared_ptr<scm::gl::text>          renderable_text = nullptr;        //renderable_text_    = scm::make_shared<scm::gl::text>(device_, output_font, font_face::style_regular, "sick, sad world...");
scm::gl::font_face_ptr                  output_font = nullptr;//(new font_face(device_, std::string(LAMURE_FONTS_DIR) + "/Ubuntu.ttf", 30, 0, font_face::smooth_lcd));




bool enable_playback = true;
float playback_speed = 1.0f;
float accumulated_playback_cursor_time = 0.0f;

float current_time_cursor_pos = 0.0f;

int32_t frame_count = 0;

int32_t num_models_ = 0;
std::vector<scm::math::mat4d> model_transformations_;

float height_divided_by_top_minus_bottom_ = 0.f;

lamure::ren::camera* camera_ = nullptr;

scm::shared_ptr<scm::gl::render_device> device_;
scm::shared_ptr<scm::gl::render_context> context_;

scm::gl::program_ptr vis_xyz_shader_;
scm::gl::program_ptr fem_vis_xyz_pass1_shader_;
scm::gl::program_ptr fem_vis_xyz_pass2_shader_;
scm::gl::program_ptr vis_xyz_pass3_shader_;

scm::gl::program_ptr vis_xyz_lighting_shader_;
scm::gl::program_ptr vis_xyz_pass2_lighting_shader_;
scm::gl::program_ptr vis_xyz_pass3_lighting_shader_;

scm::gl::program_ptr vis_xyz_qz_shader_;
scm::gl::program_ptr vis_xyz_qz_pass1_shader_;
scm::gl::program_ptr vis_xyz_qz_pass2_shader_;

scm::gl::program_ptr vis_quad_shader_;

scm::gl::program_ptr vis_trimesh_shader_;
scm::gl::program_ptr vis_trimesh_lighting_shader_;


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

scm::gl::rasterizer_state_ptr wireframe_no_backface_culling_rasterizer_state_;

scm::gl::blend_state_ptr color_blending_state_;
scm::gl::blend_state_ptr color_no_blending_state_;

scm::gl::sampler_state_ptr filter_linear_;
scm::gl::sampler_state_ptr filter_nearest_;

scm::gl::texture_2d_ptr bg_texture_;



bool changed_ssbo_simulation = true;
fem_attribute_collection g_fem_collection;

//after parsing, should contain the first simulation name of the folder the sim is contained in (e.g. Temperatur)
std::string previously_selected_FEM_simulation = "";
std::string currently_selected_FEM_simulation = "";

/* is filled with the return value of "parse_fem_collection(...)"
/ only values contained in the vector should be used for querying data from 
/ "get_data_ptr_to_simulation_data(std::string const& sim_name)"to upload into SSBO */
std::vector<std::string> successfully_parsed_simulation_names;


//ssbo containing the entire time series for the attribute of interest (e.g. Temperatur, Eigenform_001, etc.)
scm::gl::buffer_ptr fem_ssbo_time_series ;




uint64_t max_size_of_ssbo = 0;

//typedef eigenform
//CPU representation vector for eigenform values  
//std::vector<std::vector<float>> bvh_ssbo_cpu_data_;


struct resource {
  uint64_t num_primitives_ {0};
  scm::gl::buffer_ptr buffer_;
  scm::gl::vertex_array_ptr array_;
};


std::map<uint32_t, resource> bvh_resources_;
std::map<uint32_t, resource> sparse_resources_;
std::map<uint32_t, resource> frusta_resources_;
std::map<uint32_t, resource> octree_resources_;
std::map<uint32_t, resource> image_plane_resources_;
std::map<uint32_t, scm::gl::texture_2d_ptr> texture_resources_;

scm::shared_ptr<scm::gl::quad_geometry> screen_quad_;
scm::time::accum_timer<scm::time::high_res_timer> frame_time_;

double fps_ = 0.0;
uint64_t rendered_splats_ = 0;
uint64_t rendered_nodes_ = 0;

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
  bool selection_settings_ {false};
  bool view_settings_ {false};
  bool visual_settings_ {false};
  bool provenance_settings_ {false};
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

struct vertex {
    scm::math::vec3f pos_;
    scm::math::vec3f nml_;
    scm::math::vec2f uv_;
};

struct selection {
  int32_t selected_model_ = -1;
  int32_t selected_view_ = -1;
  std::vector<xyz> brush_;
  std::set<uint32_t> selected_views_;
  int64_t brush_end_{0};
};

selection selection_;

struct provenance {
  uint32_t num_views_ {0};
};

std::map<uint32_t, provenance> provenance_;


settings settings_;


scm::math::mat4d load_matrix(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
      std::cerr << "Unable to open transformation file: \"" 
          << filename << "\"\n";
      return scm::math::mat4d::identity();
  }
  scm::math::mat4d mat = scm::math::mat4d::identity();
  std::string matrix_values_string;
  std::getline(file, matrix_values_string);
  std::stringstream sstr(matrix_values_string);
  for (int i = 0; i < 16; ++i)
      sstr >> std::setprecision(16) >> mat[i];
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
            else if (key == "tex") {
              settings.textures_[address] = value;
              std::cout << "found texture for model id " << address << std::endl;
            }
            else if (key == "depth") {
              settings.min_lod_depths_[address] = atoi(value.c_str());
              std::cout << "found depth for model id " << address << std::endl;
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
            settings.aux_point_size_ = std::min(std::max(atof(value.c_str()), 0.00001), 1.0);
          }
          else if (key == "aux_point_distance") {
            settings.aux_point_distance_ = std::min(std::max(atof(value.c_str()), 0.00001), 1.0);
          }
          else if (key == "aux_focal_length") {
            settings.aux_focal_length_ = std::min(std::max(atof(value.c_str()), 0.001), 10.0);
          }
          else if (key == "max_brush_size") {
            settings.max_brush_size_ = std::min(std::max(atoi(value.c_str()), 64), 1024*1024);
          }
          else if (key == "lod_error") {
            settings.lod_error_ = std::min(std::max(atof(value.c_str()), 0.0), 10.0);
          }
          else if (key == "provenance") {
            settings.provenance_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "fem_value_mapping_file") {
            settings.fem_value_mapping_file_ = value.c_str();
          }
          else if (key == "create_aux_resources") {
            settings.create_aux_resources_ = (bool)std::max(atoi(value.c_str()), 0);
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
          else if (key == "show_photos") {
            settings.show_photos_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "show_octrees") {
            settings.show_octrees_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "show_bvhs") {
            settings.show_bvhs_ = (bool)std::max(atoi(value.c_str()), 0);
          }
          else if (key == "show_pvs") {
            settings.show_pvs_ = (bool)std::max(atoi(value.c_str()), 0);
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
          else if (key == "selection") {
            settings.selection_ = value;
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
          else if (key == "max_radius") {
            settings.max_radius_ = std::max(atof(value.c_str()), 0.0);
          }
          else if (key == "fem_to_pcl_transform") {
            std::cout << "TRANSFORM STRING: " <<  value << std::endl;
            std::istringstream in_transform(value);

            for(int element_idx = 0; element_idx < 16; ++element_idx) {
              in_transform >> settings.fem_to_pcl_transform_[element_idx]; 
            }
            //settings.width_ = std::max(atoi(value.c_str()), 64);
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
  shader->uniform("max_radius", settings_.max_radius_);

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

  if(fem_vis_xyz_pass1_shader_ == shader || fem_vis_xyz_pass2_shader_ == shader){
    shader->uniform("fem_result", settings_.fem_result_);
    shader->uniform("fem_vis_mode", settings_.fem_vis_mode_);
    shader->uniform("fem_deform_factor", settings_.fem_deform_factor_);




    //std::cout << "NUM VERTICES IN MODEL: " <<  num_vertices_in_fem_model << "\n";

    int32_t num_vertices_in_current_simulation = g_fem_collection.get_num_vertices_per_simulation(currently_selected_FEM_simulation);

   
    /*
    std::cout << "VERTICES IN SIMULATION: " 
              << g_fem_collection.get_num_vertices_per_simulation(currently_selected_FEM_simulation) << std::endl;

    std::cout << "NUM TIMESTEPS IN SIMULATION " <<
              g_fem_collection.get_num_timesteps_per_simulation(currently_selected_FEM_simulation) << std::endl;
    */


    //std::cout << "SELECTED SIMULATION: "  << currently_selected_FEM_simulation << std::endl;
    auto const extrema_current_color_attribute 
      = g_fem_collection.get_global_extrema_for_attribute_in_series(FEM_attrib::SIG_XX, currently_selected_FEM_simulation);


    //std::cout << " NUM VERTICES IN CURRENT SIMULATION: " << num_vertices_in_current_simulation << "\n";
    shader->uniform("num_vertices_in_fem", num_vertices_in_current_simulation);

    //std::cout << "Currently used min and max values for color attrib: [" 
    //          << extrema_current_color_attribute.first << "], [" << extrema_current_color_attribute.second << "]" << std::endl; 

    shader->uniform("current_min_color_attrib", extrema_current_color_attribute.first);
    shader->uniform("current_max_color_attrib", extrema_current_color_attribute.second);

    int32_t max_timestep_id = g_fem_collection.get_num_timesteps_per_simulation(currently_selected_FEM_simulation) - 1;
    shader->uniform("max_timestep_id", max_timestep_id);

    int32_t const num_fem_attributes = int(FEM_attrib::NUM_FEM_ATTRIBS);

    shader->uniform("num_attributes_in_fem", num_fem_attributes);


    int current_attribute_id = int(FEM_attrib::SIG_XX);

    shader->uniform("current_attribute_id", current_attribute_id);


    std::cout << accumulated_playback_cursor_time << std::endl;

    if(enable_playback) {
      current_time_cursor_pos = accumulated_playback_cursor_time / 1000.0f;// (frame_count) * 3.5f;
      while(current_time_cursor_pos > max_timestep_id) {
        current_time_cursor_pos -= max_timestep_id;
      }
    }

    float clamped_time_cursor_pos = current_time_cursor_pos;

      if(max_timestep_id != 0) {
        while(clamped_time_cursor_pos > max_timestep_id) {
         clamped_time_cursor_pos -= max_timestep_id;
        } 
      } else {
        clamped_time_cursor_pos = 0.0f;
      }
    


    shader->uniform("time_cursor_pos", clamped_time_cursor_pos);



    if(settings_.fem_result_ > 0){
      //std::cout << fem_md[settings_.fem_result_ - 1] << std::endl;
      shader->uniform("fem_min_absolute_deform", fem_md[settings_.fem_result_ - 1].min_absolute_deform);
      shader->uniform("fem_max_absolute_deform", fem_md[settings_.fem_result_ - 1].max_absolute_deform);
      shader->uniform("fem_min_deformation", fem_md[settings_.fem_result_ - 1].min_deformation);
      shader->uniform("fem_max_deformation", fem_md[settings_.fem_result_ - 1].max_deformation);
    }
    

  }
  
}





void draw_resources(const lamure::context_t context_id, const lamure::view_t view_id) {

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


  }

}

void draw_all_models(const lamure::context_t context_id, const lamure::view_t view_id, scm::gl::program_ptr shader, lamure::ren::bvh::primitive_type _type) {

  lamure::ren::controller* controller = lamure::ren::controller::get_instance();
  lamure::ren::cut_database* cuts = lamure::ren::cut_database::get_instance();
  lamure::ren::model_database* database = lamure::ren::model_database::get_instance();


  context_->bind_vertex_array(controller->get_context_memory(context_id, _type, device_));
  
  context_->apply();

  rendered_splats_ = 0;
  rendered_nodes_ = 0;

  for (int32_t model_id = 0; model_id < num_models_; ++model_id) {
    
    if (settings_.models_[model_id].substr(settings_.models_[model_id].size()-3) != "bvh") {
      continue;
    }

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
    if (bvh->get_primitive() != _type) {
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

    const scm::math::mat4d viewport_scale = scm::math::make_scale(render_width_ * 0.5, render_width_ * 0.5, 0.5);
    const scm::math::mat4d viewport_translate = scm::math::make_translation(1.0,1.0,1.0);
    const scm::math::mat4d model_to_screen = viewport_scale * viewport_translate * model_view_projection_matrix;
    shader->uniform("model_to_screen_matrix", scm::math::mat4f(model_to_screen));

    //scm::math::vec4d x_unit_vec = scm::math::vec4d(1.0,0.0,0.0,0.0);
    //float model_radius_scale = scm::math::length(scm::math::vec3d(model_matrix * x_unit_vec));
    //shader->uniform("model_radius_scale", model_radius_scale);
    shader->uniform("model_radius_scale", 1.f);

    size_t surfels_per_node = database->get_primitives_per_node();
    std::vector<scm::gl::boxf>const & bounding_box_vector = bvh->get_bounding_boxes();
    
    scm::gl::frustum frustum_by_model = camera_->get_frustum_by_model(scm::math::mat4f(model_matrix));
    
    for(auto const& node_slot_aggregate : renderable) {
      uint32_t node_culling_result = camera_->cull_against_frustum(
        frustum_by_model,
        bounding_box_vector[node_slot_aggregate.node_id_]);
        
      if (node_culling_result != 1) {


        if (settings_.show_accuracy_) {
          const float accuracy = 1.0 - (bvh->get_depth_of_node(node_slot_aggregate.node_id_) * 1.0)/(bvh->get_depth() - 1);
          shader->uniform("accuracy", accuracy);
        }
        if (settings_.show_radius_deviation_) {
          shader->uniform("average_radius", bvh->get_avg_primitive_extent(node_slot_aggregate.node_id_));
        }


        if("" != settings_.textures_[model_id]) {
          context_->bind_texture(texture_resources_[model_id], filter_linear_, 10);
          shader->uniform_sampler("in_mesh_color_texture", 10);

          shader->uniform("color_rendering_mode", settings_.color_rendering_mode);
        }

        context_->apply();

        if (draw) {
          switch (_type) {
            case lamure::ren::bvh::primitive_type::POINTCLOUD:
              context_->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST,
                (node_slot_aggregate.slot_id_) * (GLsizei)surfels_per_node, surfels_per_node);
              break;

            case lamure::ren::bvh::primitive_type::TRIMESH:
              context_->draw_arrays(scm::gl::PRIMITIVE_TRIANGLE_LIST,
                (node_slot_aggregate.slot_id_) * (GLsizei)surfels_per_node, surfels_per_node);
              break;

            default: break;
          }
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


void glut_display() {



  if (rendering_) {
    return;
  }
  rendering_ = true;


  std::chrono::time_point<std::chrono::system_clock> frame_start, frame_end;

  frame_start = std::chrono::system_clock::now();



  

  camera_->set_projection_matrix(settings_.fov_, float(settings_.width_)/float(settings_.height_),  settings_.near_plane_, settings_.far_plane_);


  lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
  lamure::ren::cut_database* cuts = lamure::ren::cut_database::get_instance();
  lamure::ren::controller* controller = lamure::ren::controller::get_instance();


  controller->reset_system();

  lamure::context_t context_id = controller->deduce_context_id(0);
  


  //std::cout << "Current FEM-Attribute: " << currently_selected_FEM_simulation << std::endl;

  if( (nullptr == fem_ssbo_time_series) ) {
    std::cout << "need to allocate ssbo!" << std::endl;

    fem_ssbo_time_series = device_->create_buffer(scm::gl::BIND_STORAGE_BUFFER,
                                                  scm::gl::USAGE_DYNAMIC_DRAW,
                                                  max_size_of_ssbo,
                                                  0);



  }




  if(changed_ssbo_simulation) { 
    refresh_ssbo_data();
  }



  for (lamure::model_t model_id = 0; model_id < num_models_; ++model_id) {

    if (settings_.models_[model_id].substr(settings_.models_[model_id].size()-3) != "bvh") {
      continue;
    }

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



  if (settings_.lod_update_) {
    controller->dispatch(context_id, device_); 
  }
  lamure::view_t view_id = controller->deduce_view_id(context_id, camera_->view_id());


  context_->set_rasterizer_state(no_backface_culling_rasterizer_state_);

  if (settings_.splatting_) {
    //2 pass splatting
    //PASS 1

    context_->clear_color_buffer(pass1_fbo_ , 0, scm::math::vec4f( .0f, .0f, .0f, 0.0f));
    context_->clear_depth_stencil_buffer(pass1_fbo_);
    context_->set_frame_buffer(pass1_fbo_);
      
    context_->bind_program(fem_vis_xyz_pass1_shader_);
    context_->set_blend_state(color_no_blending_state_);
    context_->set_depth_stencil_state(depth_state_less_);

    set_uniforms(fem_vis_xyz_pass1_shader_);

    context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(render_width_, render_height_)));
    context_->apply();

    draw_all_models(context_id, view_id, fem_vis_xyz_pass1_shader_, lamure::ren::bvh::primitive_type::POINTCLOUD);

    //PASS 2

    context_->clear_color_buffer(pass2_fbo_ , 0, scm::math::vec4f( .0f, .0f, .0f, 0.0f));
    context_->clear_color_buffer(pass2_fbo_ , 1, scm::math::vec4f( .0f, .0f, .0f, 0.0f));
    context_->clear_color_buffer(pass2_fbo_ , 2, scm::math::vec4f( .0f, .0f, .0f, 0.0f));
    
    context_->set_frame_buffer(pass2_fbo_);

    context_->set_blend_state(color_blending_state_);
    context_->set_depth_stencil_state(depth_state_without_writing_);
    context_->set_rasterizer_state(no_backface_culling_rasterizer_state_);

    auto selected_pass2_shading_program = fem_vis_xyz_pass2_shader_;

    if(settings_.enable_lighting_) {
      selected_pass2_shading_program = vis_xyz_pass2_lighting_shader_;
    }

    context_->bind_program(selected_pass2_shading_program);

    selected_pass2_shading_program->storage_buffer("fem_data_array_struct", 10);
    context_->bind_storage_buffer(fem_ssbo_time_series , 10);
    context_->apply();

    set_uniforms(selected_pass2_shading_program);

    context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(render_width_, render_height_)));
    context_->apply();

    draw_all_models(context_id, view_id, selected_pass2_shading_program, lamure::ren::bvh::primitive_type::POINTCLOUD);

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

    //draw pointclouds
    auto selected_single_pass_shading_program = vis_xyz_shader_;

    if(settings_.enable_lighting_) {
      selected_single_pass_shading_program = vis_xyz_lighting_shader_;
    }

    context_->bind_program(selected_single_pass_shading_program);
    context_->set_blend_state(color_no_blending_state_);
    context_->set_depth_stencil_state(depth_state_less_);
    
    set_uniforms(selected_single_pass_shading_program);
    context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(render_width_, render_height_)));
    context_->apply();
    draw_all_models(context_id, view_id, selected_single_pass_shading_program, lamure::ren::bvh::primitive_type::POINTCLOUD);

    //draw meshes
    

    selected_single_pass_shading_program = vis_trimesh_shader_;
    if(settings_.enable_lighting_) {
      selected_single_pass_shading_program = vis_trimesh_lighting_shader_;
    }
    context_->bind_program(selected_single_pass_shading_program);

    set_uniforms(selected_single_pass_shading_program);
    context_->apply();

#if 1
    //draw shaded
    draw_all_models(context_id, view_id, selected_single_pass_shading_program, lamure::ren::bvh::primitive_type::TRIMESH);
#else
    //draw wireframe
    context_->set_rasterizer_state(wireframe_no_backface_culling_rasterizer_state_);
    context_->apply();
    draw_all_models(context_id, view_id, selected_single_pass_shading_program, lamure::ren::bvh::primitive_type::TRIMESH);
#endif

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

  frame_time_.stop();
  frame_time_.start();

  //schism bug ? time::to_seconds yields milliseconds

  //std::cout << "Frame time: " << scm::time::to_seconds(frame_time_.accumulated_duration()); 
  if (scm::time::to_seconds(frame_time_.accumulated_duration()) > 100.0) {
    fps_ = 1000.0f / scm::time::to_seconds(frame_time_.average_duration());

    ++frame_count;
    frame_time_.reset();
  }
  

  frame_end = std::chrono::system_clock::now();


  float elapsed_milliseconds = std::chrono::duration_cast<std::chrono::microseconds>
                           (frame_end-frame_start).count() / 1000.0;


  if(enable_playback) {
    accumulated_playback_cursor_time += elapsed_milliseconds;
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

  camera_->set_projection_matrix(settings_.fov_, float(settings_.width_)/float(settings_.height_),  settings_.near_plane_, settings_.far_plane_);

  gui_.ortho_matrix_ = 
    scm::math::make_ortho_matrix(0.0f, static_cast<float>(settings_.width_),
    0.0f, static_cast<float>(settings_.height_), -1.0f, 1.0f);

}





void glut_keyboard(unsigned char key, int32_t x, int32_t y) {

  int k = (int)key;
  std::cout << "pressed " << key << std::endl;
  switch (k) {
    case 27:
      exit(0);
      break;


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
      settings_.fem_result_ = 0;
      break;
    case '1':
    {

      if(! (successfully_parsed_simulation_names.size() > 0) ) {
        break;
      }

      std::string const& newly_selected_simulation = successfully_parsed_simulation_names[0];

      if(newly_selected_simulation != currently_selected_FEM_simulation) {
        currently_selected_FEM_simulation = newly_selected_simulation;
        changed_ssbo_simulation = true;

        std::cout << "Changed FEM data to simulation: " << currently_selected_FEM_simulation << std::endl;
      }

      settings_.fem_result_ = 1;
      break;
    }
    case '2':
    {
      if(! (successfully_parsed_simulation_names.size() > 1) ) {
        break;
      }
      std::string const& newly_selected_simulation = successfully_parsed_simulation_names[1];
      if(newly_selected_simulation != currently_selected_FEM_simulation) {
        currently_selected_FEM_simulation = newly_selected_simulation;
        changed_ssbo_simulation = true;

        std::cout << "Changed FEM data to simulation: " << currently_selected_FEM_simulation << std::endl;
      }

      settings_.fem_result_ = 1;
      break;
    }
    case '3':
    {
      if(! (successfully_parsed_simulation_names.size() > 2) ) {
        break;
      }
      std::string const& newly_selected_simulation = successfully_parsed_simulation_names[2];
      if(newly_selected_simulation != currently_selected_FEM_simulation) {
        currently_selected_FEM_simulation = newly_selected_simulation;
        changed_ssbo_simulation = true;

        std::cout << "Changed FEM data to simulation: " << currently_selected_FEM_simulation << std::endl;
      }

      settings_.fem_result_ = 1;
      break;
    }
    case '4':
    {
      if(! (successfully_parsed_simulation_names.size() > 3) ) {
        break;
      }
      std::string const& newly_selected_simulation = successfully_parsed_simulation_names[3];
      if(newly_selected_simulation != currently_selected_FEM_simulation) {
        currently_selected_FEM_simulation = newly_selected_simulation;
        changed_ssbo_simulation = true;

        std::cout << "Changed FEM data to simulation: " << currently_selected_FEM_simulation << std::endl;
      }

      settings_.fem_result_ = 1;
      break;
    }
    case '5':
    {
      if(! (successfully_parsed_simulation_names.size() > 4) ) {
        break;
      }
      std::string const& newly_selected_simulation = successfully_parsed_simulation_names[4];
      if(newly_selected_simulation != currently_selected_FEM_simulation) {
        currently_selected_FEM_simulation = newly_selected_simulation;
        changed_ssbo_simulation = true;

        std::cout << "Changed FEM data to simulation: " << currently_selected_FEM_simulation << std::endl;
      }

      settings_.fem_result_ = 1;
      break;
    }
    case '6':
    {
      if(! (successfully_parsed_simulation_names.size() > 5) ) {
        break;
      }
      std::string const& newly_selected_simulation = successfully_parsed_simulation_names[5];
      if(newly_selected_simulation != currently_selected_FEM_simulation) {
        currently_selected_FEM_simulation = newly_selected_simulation;
        changed_ssbo_simulation = true;

        std::cout << "Changed FEM data to simulation: " << currently_selected_FEM_simulation << std::endl;
      }

      settings_.fem_result_ = 1;
      break;
    }
    case '7':
    {
      if(! (successfully_parsed_simulation_names.size() > 6) ) {
        break;
      }
      std::string const& newly_selected_simulation = successfully_parsed_simulation_names[6];
      if(newly_selected_simulation != currently_selected_FEM_simulation) {
        currently_selected_FEM_simulation = newly_selected_simulation;
        changed_ssbo_simulation = true;

        std::cout << "Changed FEM data to simulation: " << currently_selected_FEM_simulation << std::endl;
      }

      settings_.fem_result_ = 1;
      break;
    }
    case '8':
    {
      if(! (successfully_parsed_simulation_names.size() > 7) ) {
        break;
      }
      std::string const& newly_selected_simulation = successfully_parsed_simulation_names[7];
      if(newly_selected_simulation != currently_selected_FEM_simulation) {
        currently_selected_FEM_simulation = newly_selected_simulation;
        changed_ssbo_simulation = true;

        std::cout << "Changed FEM data to simulation: " << currently_selected_FEM_simulation << std::endl;
      }

      settings_.fem_result_ = 1;
      break;
    }
    case '9':
    {
      if(! (successfully_parsed_simulation_names.size() > 8) ) {
        break;
      }
      std::string const& newly_selected_simulation = successfully_parsed_simulation_names[8];
      if(newly_selected_simulation != currently_selected_FEM_simulation) {
        currently_selected_FEM_simulation = newly_selected_simulation;
        changed_ssbo_simulation = true;

        std::cout << "Changed FEM data to simulation: " << currently_selected_FEM_simulation << std::endl;
      }

      settings_.fem_result_ = 1;
      break;
    }

    case 'D':
      settings_.fem_vis_mode_ = !settings_.fem_vis_mode_;
      break;
    case 'R':
      settings_.fem_deform_factor_ = 1.0;
      break;

    case 'P':
      enable_playback = !enable_playback;
      break;

    case 'U':
      settings_.fem_deform_factor_ *= 1.01;
      break;
    case 'J':
      settings_.fem_deform_factor_ *= 0.99;
      break;


    case ' ':
      settings_.gui_ = !settings_.gui_;
      break;


    default:
      break;

  }

  std::cout << "fem settings: " << std::endl;
  std::cout << "fem_result_: " << settings_.fem_result_ << std::endl;
  std::cout << "fem_vis_mode_: " << settings_.fem_vis_mode_ << " use a to toggle between blending and deformation" << std::endl;
  std::cout << "fem_deform_factor_: " << settings_.fem_deform_factor_ <<  " use u/j for decrease/increase" << std::endl;
}


void glut_motion(int32_t x, int32_t y) {

  if (input_.gui_lock_) {
    input_.prev_mouse_ = scm::math::vec2i(x, y);
    input_.mouse_ = scm::math::vec2i(x, y);
    return;
  }

  input_.prev_mouse_ = input_.mouse_;
  input_.mouse_ = scm::math::vec2i(x, y);
  

  camera_->update_trackball(x, y, settings_.width_, settings_.height_, input_.mouse_state_);
  

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
        case GLFW_KEY_T:
        {
          int const NUM_COLOR_MODES = 2;
          settings_.color_rendering_mode = (settings_.color_rendering_mode + 1) % NUM_COLOR_MODES;
          break;
        }
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
  wireframe_no_backface_culling_rasterizer_state_ = device_->create_rasterizer_state(scm::gl::FILL_WIREFRAME, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, false, false, 0.0, false, false);
  filter_linear_  = device_->create_sampler_state(scm::gl::FILTER_ANISOTROPIC, scm::gl::WRAP_CLAMP_TO_EDGE, 16u);
  filter_nearest_ = device_->create_sampler_state(scm::gl::FILTER_MIN_MAG_LINEAR, scm::gl::WRAP_CLAMP_TO_EDGE);
}


void init_camera() {

  scm::math::vec3f root_bb_min(-1.f, -1.f, -1.f);
  scm::math::vec3f root_bb_max(1.f, 1.f, 1.f);
  scm::math::vec3f center(0.f, 0.f, 0.f);

  if (num_models_ > 0 
    && settings_.models_[0].substr(settings_.models_[0].size()-3) == "bvh") {

    auto root_bb = lamure::ren::model_database::get_instance()->get_model(0)->get_bvh()->get_bounding_boxes()[0];
    root_bb_min = scm::math::mat4f(model_transformations_[0]) * root_bb.min_vertex();
    root_bb_max = scm::math::mat4f(model_transformations_[0]) * root_bb.max_vertex();
    center = (root_bb_min + root_bb_max) / 2.f;

  }

  camera_ = new lamure::ren::camera(0,
                                    make_look_at_matrix(center + scm::math::vec3f(0.f, 0.1f, -0.01f), center, scm::math::vec3f(0.f, 1.f, 0.f)),
                                    length(root_bb_max-root_bb_min), false, false);
  
  camera_->set_dolly_sens_(settings_.travel_speed_);

  if (settings_.use_view_tf_) {
    camera_->set_view_matrix(settings_.view_tf_);
    std::cout << "view_tf:" << std::endl;
    std::cout << camera_->get_high_precision_view_matrix() << std::endl;
    camera_->set_dolly_sens_(settings_.travel_speed_);
  }

  camera_->set_projection_matrix(settings_.fov_, float(settings_.width_)/float(settings_.height_),  settings_.near_plane_, settings_.far_plane_);

  screen_quad_.reset(new scm::gl::quad_geometry(device_, scm::math::vec2f(-1.0f, -1.0f), scm::math::vec2f(1.0f, 1.0f)));

  gui_.ortho_matrix_ = scm::math::make_ortho_matrix(0.0f, static_cast<float>(settings_.width_),
                                                    0.0f, static_cast<float>(settings_.height_), -1.0f, 1.0f);
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
 
    
    std::string vis_trimesh_vs_source;
    std::string vis_trimesh_fs_source;
    std::string vis_trimesh_vs_lighting_source;
    std::string vis_trimesh_fs_lighting_source;


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
      || !read_shader(shader_root_path + "/vis/vis_trimesh.glslv", vis_trimesh_vs_source)
      || !read_shader(shader_root_path + "/vis/vis_trimesh.glslf", vis_trimesh_fs_source)
      || !read_shader(shader_root_path + "/vis/vis_trimesh.glslv", vis_trimesh_vs_lighting_source, true)
      || !read_shader(shader_root_path + "/vis/vis_trimesh.glslf", vis_trimesh_fs_lighting_source, true)

      || !read_shader(shader_root_path + "/vis/vis_xyz.glslv", vis_xyz_vs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz.glslg", vis_xyz_gs_source)
      || !read_shader(shader_root_path + "/vis/vis_xyz.glslf", vis_xyz_fs_source)
      || !read_shader(shader_root_path + "/fem_vis/fem_vis_ssbo_xyz_pass1.glslv", vis_xyz_pass1_vs_source)
      || !read_shader(shader_root_path + "/fem_vis/fem_vis_xyz_pass1.glslg", vis_xyz_pass1_gs_source)
      || !read_shader(shader_root_path + "/fem_vis/fem_vis_xyz_pass1.glslf", vis_xyz_pass1_fs_source)
      || !read_shader(shader_root_path + "/fem_vis/fem_vis_ssbo_xyz_pass2.glslv", vis_xyz_pass2_vs_source)
      || !read_shader(shader_root_path + "/fem_vis/fem_vis_xyz_pass2.glslg", vis_xyz_pass2_gs_source)
      || !read_shader(shader_root_path + "/fem_vis/fem_vis_xyz_pass2.glslf", vis_xyz_pass2_fs_source)
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



    vis_trimesh_shader_ = device_->create_program(
            boost::assign::list_of
                    (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_trimesh_vs_source))
                    (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_trimesh_fs_source)));
    if (!vis_trimesh_shader_) {
      std::cout << "error creating shader vis_trimesh_shader_ program" << std::endl;
      std::exit(1);
    }

    vis_trimesh_lighting_shader_ = device_->create_program(
            boost::assign::list_of
                    (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_trimesh_vs_lighting_source))
                    (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_trimesh_fs_lighting_source)));
    if (!vis_trimesh_lighting_shader_) {
      std::cout << "error creating shader vis_trimesh_lighting_shader_ program" << std::endl;
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

    fem_vis_xyz_pass1_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_pass1_vs_source))
        (device_->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, vis_xyz_pass1_gs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_pass1_fs_source)));
    if (!fem_vis_xyz_pass1_shader_) {
      std::cout << "error creating fem_vis_xyz_pass1_shader_ program" << std::endl;
      exit(1);
    }

    fem_vis_xyz_pass2_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vis_xyz_pass2_vs_source))
        (device_->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, vis_xyz_pass2_gs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vis_xyz_pass2_fs_source)));
    if (!fem_vis_xyz_pass2_shader_) {
      std::cout << "error creating fem_vis_xyz_pass2_shader_ program" << std::endl;
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


  }
  catch (std::exception& e)
  {
      std::cout << e.what() << std::endl;
  }



  create_framebuffers();



  init_render_states();
  init_camera();



  if (settings_.background_image_ != "") {
    //std::cout << "background image: " << settings_.background_image_ << std::endl;
    scm::gl::texture_loader tl;
    bg_texture_ = tl.load_texture_2d(*device_, settings_.background_image_, true, false);
  }



}

std::string
make_short_name(const std::string& s){
  const unsigned max_length = 36;
  if(s.length() > max_length){
    std::string shortname = s.substr(s.length() - 36, 36);
    return shortname;
  }
  return s;

}

void gui_selection_settings(settings& stgs){

    ImGui::SetNextWindowPos(ImVec2(20, 315));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 220.0f));

    ImGui::Begin("Selection", &gui_.selection_settings_, ImGuiWindowFlags_MenuBar);

    std::vector<std::string> model_names_short;
    for(const auto& s : stgs.models_){
      model_names_short.push_back(make_short_name(s));
    }

    char** model_names = new char* [num_models_ + 1];
    for(unsigned i = 0; i < model_names_short.size(); ++i ){
      model_names[i] = ((char *) model_names_short[i].c_str());
    }
    std::string all("All");
    model_names[num_models_] = (char *) all.c_str();

    static int32_t dataset = selection_.selected_model_;
    if (selection_.selected_model_ == -1) {
      dataset = num_models_;
    }

    ImGui::Combo("Dataset", &dataset, (const char* const*)model_names, num_models_+1);

    if(dataset == num_models_){
      selection_.selected_model_ = -1;
    } else {
      selection_.selected_model_ = dataset;
    }

    if (settings_.create_aux_resources_ && settings_.atlas_file_ != "") {
      if (ImGui::Button("Cycle Images")) {
        if (selection_.selected_model_ != -1) {
          if (settings_.views_.find(selection_.selected_model_) != settings_.views_.end()) {
            selection_.selected_view_ = (selection_.selected_view_+1) % settings_.views_[selection_.selected_model_].size();
            const auto& view = settings_.views_[selection_.selected_model_][selection_.selected_view_];
            auto camera_matrix = view.transform_;
            camera_matrix = camera_matrix 
              //* scm::math::make_translation(0.f, 0.f, settings_.aux_point_size_*10.f) 
              * scm::math::make_rotation(180.f, 0.f, 0.f, 1.f);
            camera_->set_view_matrix(scm::math::mat4d(scm::math::inverse(camera_matrix)));
            camera_->set_projection_matrix(settings_.fov_, float(settings_.width_)/float(settings_.height_),  settings_.near_plane_, settings_.far_plane_);
            selection_.selected_views_.clear();
            selection_.selected_views_.insert(selection_.selected_view_);
            settings_.show_views_ = true;
            settings_.splatting_ = false;
          }
        }
      }
    }
    else {
      ImGui::Text("No atlas file");
    }
    ImGui::Checkbox("Brush", &input_.brush_mode_);

    ImGui::Text("Selection: %d / %d", (int32_t)selection_.brush_end_, (int32_t)settings_.max_brush_size_);
    if (settings_.create_aux_resources_ && settings_.atlas_file_ != "") {
      if (selection_.selected_model_ != -1 && selection_.selected_views_.size() == 1) {
        ImGui::Text("Image: %d %s", (int32_t)selection_.selected_view_, 
          settings_.views_[selection_.selected_model_][selection_.selected_view_].image_file_.c_str());
      }
      else {
        ImGui::Text("Images: %d", (int32_t)selection_.selected_views_.size());
      }
    }

    if (ImGui::Button("Clear Selection")) {
      selection_.selected_views_.clear();
      selection_.brush_end_ = 0;
      input_.brush_clear_ = false;
    }

    ImGui::End();

    delete [] model_names;
}


void gui_view_settings(){
    ImGui::SetNextWindowPos(ImVec2(20, 555));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 335.0f));
    ImGui::Begin("View / LOD Settings", &gui_.view_settings_, ImGuiWindowFlags_MenuBar);
    //if (ImGui::SliderFloat("Near Plane", &settings_.near_plane_, 0, 1.0f, "%.4f", 4.0f)) {
    //  input_.gui_lock_ = true;
    //}
    //if (ImGui::SliderFloat("Far Plane", &settings_.far_plane_, 0, 1000.0f, "%.4f", 4.0f)) {
    //  input_.gui_lock_ = true;
    //}
    if (ImGui::SliderFloat("Travel Speed", &settings_.travel_speed_, 0.01f, 300.0f, "%.4f", 4.0f)) {
      input_.gui_lock_ = true;
    }
    if (ImGui::SliderFloat("FOV", &settings_.fov_, 18, 90.0f)) {
      input_.gui_lock_ = true;
    }

    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
    
    ImGui::Checkbox("Lod Update", &settings_.lod_update_);
    if (settings_.create_aux_resources_) {
      ImGui::Checkbox("Show BVHs", &settings_.show_bvhs_);
      if (settings_.show_bvhs_) {
        settings_.splatting_ = false;
      }
    }

    if (ImGui::SliderFloat("LOD Error", &settings_.lod_error_, 0.1f, 50.0f, "%.2f", 2.5f)) {
      input_.gui_lock_ = true;
    }
    if (ImGui::SliderFloat("LOD Point Scale", &settings_.lod_point_scale_, 0.1f, 2.0f, "%.4f", 1.0f)) {
      input_.gui_lock_ = true;
    }
    
    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();



    ImGui::End();
}


void gui_visual_settings(){

    size_t data_provenance_size_in_bytes = lamure::ren::data_provenance::get_instance()->get_size_in_bytes();

    uint32_t num_attributes = 5 + data_provenance_size_in_bytes/sizeof(float);

    const char* vis_values[] = {
      "Color", "Normals", "Accuracy", 
      "Radius Deviation", "Output Sensitivity", 
      "Provenance 1", "Provenance 2", "Provenance 3", 
      "Provenance 4", "Provenance 5", "Provenance 6", "Provenance 7" };
    static int it = settings_.vis_;

    ImGui::SetNextWindowPos(ImVec2(settings_.width_-520, 20));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 305.0f));
    ImGui::Begin("Visual Settings", &gui_.visual_settings_, ImGuiWindowFlags_MenuBar);
    
    uint32_t num_vis_entries = (5 + data_provenance_size_in_bytes/sizeof(float));
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
    ImGui::Checkbox("Enable Lighting", &settings_.enable_lighting_);
    ImGui::Checkbox("Use Material Color", &settings_.use_material_color_);
    ImGui::Checkbox("Gamma Correction", &settings_.gamma_correction_);
    
    static ImVec4 color_mat_diff = ImColor(0.6f, 0.6f, 0.6f, 1.0f);
    ImGui::Text("Material Diffuse");
    ImGui::ColorEdit3("Diffuse", (float*)&color_mat_diff, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoAlpha);
    settings_.material_diffuse_.x = color_mat_diff.x;
    settings_.material_diffuse_.y = color_mat_diff.y;
    settings_.material_diffuse_.z = color_mat_diff.z;

    static ImVec4 background_color = ImColor(settings_.background_color_.x, settings_.background_color_.y, settings_.background_color_.z, 1.0f);
    ImGui::Text("Background Color");
    ImGui::ColorEdit3("Background", (float*)&background_color, ImGuiColorEditFlags_Float);
    settings_.background_color_.x = background_color.x;
    settings_.background_color_.y = background_color.y;
    settings_.background_color_.z = background_color.z;

    ImGui::End();
}


void gui_provenance_settings(){
    
    ImGui::SetNextWindowPos(ImVec2(settings_.width_-520, 345));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 450.0f));
    ImGui::Begin("Provenance Settings", &gui_.provenance_settings_, ImGuiWindowFlags_MenuBar);

    if (ImGui::SliderFloat("AUX Point Size", &settings_.aux_point_size_, 0.1f, 10.0f, "%.4f", 4.0f)) {
      input_.gui_lock_ = true;
    }
    if (ImGui::SliderFloat("AUX Point Scale", &settings_.aux_point_scale_, 0.1f, 2.0f, "%.4f", 4.0f)) {
      input_.gui_lock_ = true;
    }
    if (ImGui::SliderFloat("AUX Focal Length", &settings_.aux_focal_length_, 0.1f, 2.0f, "%.4f", 4.0f)) {
      input_.gui_lock_ = true;
    }

    if (settings_.create_aux_resources_) {
      ImGui::Checkbox("Show Sparse", &settings_.show_sparse_);
      if (settings_.show_sparse_) {
          settings_.enable_lighting_ = false;
          settings_.splatting_ = false;
      }

      ImGui::Checkbox("Show Views", &settings_.show_views_);
      if (settings_.show_views_) {
        settings_.splatting_ = false;
      }

      if (settings_.atlas_file_ != "") {
        ImGui::Checkbox("Show Photos", &settings_.show_photos_);
        if (settings_.show_photos_) {
          settings_.splatting_ = false;
        }
      }

      ImGui::Checkbox("Show Octrees", &settings_.show_octrees_);
      if (settings_.show_octrees_) {
          settings_.splatting_ = false;
      }
    }
    else {
      ImGui::Text("No aux file");
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
    static bool status_screen = false;
    
    ImGui::SetNextWindowPos(ImVec2(20, 20));
    ImGui::SetNextWindowSize(ImVec2(500.0f, 275.0f));
    ImGui::Begin("lamure_fem_vis GUI", &status_screen, ImGuiWindowFlags_MenuBar);
    ImGui::Text("fps %d", (int32_t)fps_);

    double f = (rendered_splats_ / 1000000.0);
    
    std::stringstream stream;
    stream << std::setprecision(2) << f;
    std::string s = stream.str();

    ImGui::Text("# points %s mio.", s.c_str());
    ImGui::Text("# nodes %d", (uint64_t)rendered_nodes_);
    ImGui::Text("# models %d", num_models_);
    
    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

    ImGui::Checkbox("Selection", &gui_.selection_settings_);
    ImGui::Checkbox("View / LOD Settings", &gui_.view_settings_);
    ImGui::Checkbox("Visual Settings", &gui_.visual_settings_);
    if (settings_.create_aux_resources_) {
      ImGui::Checkbox("Provenance Settings", &gui_.provenance_settings_);
    }
    else {
      ImGui::Text("No provenance file");
    }
    
    if (gui_.selection_settings_){
        gui_selection_settings(settings_);
    }

    if (gui_.view_settings_){
        gui_view_settings();
    }
      
    if (gui_.visual_settings_){
        gui_visual_settings();
    }

    if (settings_.create_aux_resources_ && gui_.provenance_settings_ && settings_.create_aux_resources_){
        gui_provenance_settings();
    }


    ImGui::End();


    ImGui::SetNextWindowPos(ImVec2(0, 1000));
    ImGui::SetNextWindowSize(ImVec2( settings_.width_, 50));
    ImGui::Begin( ("Playback: " + currently_selected_FEM_simulation).c_str() );

    ImGui::SliderFloat("Time Cursor (milliseconds)", &current_time_cursor_pos, 0.0f, 101.0f);

    ImGui::End();
}


/*
[{
  "type": "float",
  "visualization": " staticLoadcase_01 ",
  "min_absolute_deform": " 1.40008e-05 ",
  "max_absolute_deform": " 0.153093 ",
  "min_deformation": " 0.499991 0.499995 0.498599 ",
  "max_deformation": " 0.566313 0.538207 0.501795 "
},{
  "type": "float",
  "visualization": " staticLoadcase_02 ",
  "min_absolute_deform": " 2.00011e-06 ",
  "max_absolute_deform": " 0.0046979 ",
  "min_deformation": " 0.499859 0.499901 0.497656 ",
  "max_deformation": " 0.500755 0.500394 0.5 "
}]
*/


bool parse_json_for_fem(const std::string& filename){



  std::ifstream ifs(filename);
  std::string json((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

  picojson::value v;
  std::string err = picojson::parse(v, json);
  if(!err.empty())
  {
      std::cout << err << std::endl;
      return false;
  }

  // check if the type of the value is "array"
  if(!v.is<picojson::array>())
  {
      std::cout << "JSON is not an array" << std::endl;
      return false;
  }

  // obtain a const reference to the map, and print the contents
  const picojson::value::array &array = v.get<picojson::array>();
  for(picojson::value::array::const_iterator iter = array.begin(); iter != array.end(); ++iter){

      picojson::value::object obj = (*iter).get<picojson::value::object>();

/*
struct fem_result_meta_data{
  std::string name;
  float min_absolute_deform;
  float max_absolute_deform;
  scm::math::vec3f min_deformation;
  scm::math::vec3f max_deformation;
};
std::vector<fem_result_meta_data> fem_md;

*/


      fem_result_meta_data md;

      picojson::value::object::const_iterator iter_name = obj.find("visualization");
      if(iter_name != obj.end()){
        md.name = iter_name->second.get<std::string>();
      }

      picojson::value::object::const_iterator iter_min_absolute_deform = obj.find("min_absolute_deform");
      if(iter_min_absolute_deform != obj.end()){
        std::stringstream tmp_sstr;
        tmp_sstr << iter_min_absolute_deform->second.get<std::string>();
        tmp_sstr >> md.min_absolute_deform;
      }

      picojson::value::object::const_iterator iter_max_absolute_deform = obj.find("max_absolute_deform");
      if(iter_max_absolute_deform != obj.end()){
        std::stringstream tmp_sstr;
        tmp_sstr << iter_max_absolute_deform->second.get<std::string>();
        tmp_sstr >> md.max_absolute_deform;        
      }

      picojson::value::object::const_iterator iter_min_deformation = obj.find("min_deformation");
      if(iter_min_deformation != obj.end()){
        std::stringstream tmp_sstr;
        tmp_sstr << iter_min_deformation->second.get<std::string>();
        tmp_sstr >> md.min_deformation[0] >> md.min_deformation[1] >> md.min_deformation[2]; 
      }

      picojson::value::object::const_iterator iter_max_deformation = obj.find("max_deformation");
      if(iter_max_deformation != obj.end()){
        std::stringstream tmp_sstr;
        tmp_sstr << iter_max_deformation->second.get<std::string>();
        tmp_sstr >> md.max_deformation[0] >> md.max_deformation[1] >> md.max_deformation[2];   
      }

      std::cout << md << std::endl;

      fem_md.push_back(md);    
  }
  return true;
}



int main(int argc, char *argv[])
{
    
  std::string vis_file = "";

  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " <vis_file.vis>" << std::endl;
    std::cout << "\tHELP: to render a single model use:" << std::endl;
    std::cout << "\techo <input_file.bvh> > default.vis && " << argv[0] << " default.vis" << std::endl;
    return 0;
  }
  else {
    vis_file = argv[1];
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
    lamure::ren::data_provenance::get_instance()->parse_json(settings_.json_);
    std::cout << "size of provenance: " << lamure::ren::data_provenance::get_instance()->get_size_in_bytes() << std::endl;
    parse_json_for_fem(settings_.json_);
  }
 

  if (settings_.provenance_ && settings_.json_ != "" && settings_.fem_value_mapping_file_ != "") {

    std::cout << "Starting to read mapping file!" << std::endl;
    std::cout << settings_.fem_value_mapping_file_ << std::endl;


    successfully_parsed_simulation_names = parse_fem_collection(settings_.fem_value_mapping_file_, g_fem_collection, settings_.fem_to_pcl_transform_);


    if(successfully_parsed_simulation_names.empty()) {
      throw no_FEM_simulation_parsed_exception();
    }

    currently_selected_FEM_simulation = successfully_parsed_simulation_names[0];

    std::cout << "Parsed everything" << std::endl;


    std::cout << "Max num timesteps in any series: " << g_fem_collection.get_max_num_timesteps_in_collection() << std::endl;
    std::cout << "Max num elements in any series of timesteps: " << g_fem_collection.get_max_num_elements_per_simulation() << std::endl;


    max_size_of_ssbo = g_fem_collection.get_max_num_elements_per_simulation() * sizeof(float);

    

    std::cout << "Allocating ssbo of size " << max_size_of_ssbo << std::endl;

    //return 0;
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
    
    //force single pass for trimeshes
    lamure::ren::bvh* bvh = database->get_model(model_id)->get_bvh();
    if (bvh->get_primitive() == lamure::ren::bvh::primitive_type::TRIMESH) {
      settings_.splatting_ = false;
      bvh->set_min_lod_depth(settings_.min_lod_depths_[num_models_]);
    }


    ++num_models_;
  }

  glfwSetErrorCallback(EventHandler::on_error);

  if (!glfwInit()) {
    std::runtime_error("GLFW initialisation failed");
  }

  Window *primary_window = create_window(settings_.width_, settings_.height_, "lamure_fem_vis", nullptr, nullptr);
  make_context_current(primary_window);
  glfwSwapInterval(0);

  init();

  make_context_current(primary_window);
  glut_display();

  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if (GLEW_OK != err) {
    std::cout << "GLEW error: " << glewGetErrorString(err) << std::endl;
  }
  std::cout << "Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;

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


void refresh_ssbo_data() {


      std::cout << "Loading ssbo with data from: " << currently_selected_FEM_simulation << "\n";

      int64_t num_byte_to_copy_elements_to_allocate 
        =   g_fem_collection.get_num_timesteps_per_simulation(currently_selected_FEM_simulation) 
          * g_fem_collection.get_num_vertices_per_simulation(currently_selected_FEM_simulation) 
          * sizeof(float) * g_fem_collection.get_num_attributes_per_simulation(currently_selected_FEM_simulation);

      
      float* mapped_fem_ssbo = (float*)device_->main_context()->map_buffer(fem_ssbo_time_series, scm::gl::access_mode::ACCESS_WRITE_ONLY);
      memcpy((char*) mapped_fem_ssbo, g_fem_collection.get_data_ptr_to_simulation_data(currently_selected_FEM_simulation), num_byte_to_copy_elements_to_allocate); // CHANGE MAX NUM ELEMENTS


      device_->main_context()->unmap_buffer(fem_ssbo_time_series);

      changed_ssbo_simulation = false;

}