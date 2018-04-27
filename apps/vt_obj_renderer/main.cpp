// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

//lamure
#include <lamure/types.h>
#include <lamure/ren/camera.h>
#include <lamure/ren/config.h>
#include <lamure/ren/policy.h>

//lamure vt
#include <lamure/vt/VTConfig.h>
#include <lamure/vt/ren/CutDatabase.h>
#include <lamure/vt/ren/CutUpdate.h>

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



bool rendering_ = false;
int32_t window_width_ = 1280;
int32_t window_height_ = 720;

lamure::ren::camera* camera_ = nullptr;


scm::shared_ptr<scm::gl::render_device> device_;
scm::shared_ptr<scm::gl::render_context> context_;

struct vt_info {
  uint32_t texture_id_;
  uint16_t view_id_;
  uint16_t context_id_;
  uint64_t cut_id_;
  vt::CutUpdate* cut_update_;
  std::vector<scm::gl::texture_2d_ptr> index_texture_hierarchy_;
  scm::gl::texture_2d_ptr physical_texture_;
  scm::math::vec2ui physical_texture_size_;
  scm::math::vec2ui physical_texture_tile_size_;
  size_t size_feedback_;
  std::vector<int32_t> feedback_lod_cpu_buffer_;
  std::vector<uint32_t> feedback_count_cpu_buffer_;
  scm::gl::buffer_ptr feedback_lod_storage_;
  scm::gl::buffer_ptr feedback_count_storage_;
};

vt_info vt_;

struct resource {
  uint64_t num_primitives_ {0};
  scm::gl::buffer_ptr buffer_;
  scm::gl::vertex_array_ptr array_;
};

resource mesh_;
scm::gl::program_ptr shader_;
scm::gl::program_ptr vt_shader_;

struct input {
  float trackball_x_ = 0.f;
  float trackball_y_ = 0.f;
  scm::math::vec2i mouse_;
  scm::math::vec2i prev_mouse_;
  lamure::ren::camera::mouse_state mouse_state_;
};

input input_;

struct vertex {
  scm::math::vec3f position_;
  scm::math::vec2f coords_;
  scm::math::vec3f normal_;
};

scm::gl::frame_buffer_ptr fbo_;
scm::gl::texture_2d_ptr fbo_color_buffer_;
scm::gl::texture_2d_ptr fbo_depth_buffer_;

scm::gl::blend_state_ptr color_no_blending_state_;
scm::gl::depth_stencil_state_ptr depth_state_less_;
scm::gl::rasterizer_state_ptr no_backface_culling_rasterizer_state_;
scm::gl::rasterizer_state_ptr culling_rasterizer_state_;
scm::gl::sampler_state_ptr filter_linear_;
scm::gl::sampler_state_ptr filter_nearest_;

scm::gl::data_format get_vt_tex_format() {
  switch(vt::VTConfig::get_instance().get_format_texture()) {
    case vt::VTConfig::R8:
      return scm::gl::FORMAT_R_8;
    case vt::VTConfig::RGB8:
      return scm::gl::FORMAT_RGB_8;
    case vt::VTConfig::RGBA8:
    default:
      return scm::gl::FORMAT_RGBA_8;
  }
}

void glut_display() {
  if (rendering_) {
    return;
  }
  rendering_ = true;

  context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(window_width_, window_height_)));

  //context_->clear_color_buffer(fbo_ , 0, scm::math::vec4f( .5f, .5f, .5f, 1.0f));
  //context_->clear_depth_stencil_buffer(fbo_);
  //context_->set_frame_buffer(fbo_);

  context_->clear_default_depth_stencil_buffer();
  context_->clear_default_color_buffer();
  context_->set_default_frame_buffer();

  context_->bind_program(shader_);
  context_->set_blend_state(color_no_blending_state_);
  context_->set_depth_stencil_state(depth_state_less_);
  context_->set_rasterizer_state(no_backface_culling_rasterizer_state_);

  scm::math::mat4d model_matrix = scm::math::mat4d::identity();
  scm::math::mat4d projection_matrix = scm::math::mat4d(camera_->get_projection_matrix());
  scm::math::mat4d view_matrix = camera_->get_high_precision_view_matrix();
  scm::math::mat4d model_view_matrix = view_matrix * model_matrix;
  scm::math::mat4d model_view_projection_matrix = projection_matrix * model_view_matrix;
  shader_->uniform("mvp_matrix", scm::math::mat4f(model_view_projection_matrix));
  

  uint64_t cut_id = (((uint64_t)vt_.texture_id_) << 32) | ((uint64_t)vt_.view_id_ << 16) | ((uint64_t)vt_.context_id_);
  uint32_t max_depth_level = (*vt::CutDatabase::get_instance().get_cut_map())[cut_id]->get_atlas()->getDepth() - 1;

  context_->bind_program(vt_shader_);

  vt_shader_->uniform("projection_matrix", scm::math::mat4f(projection_matrix));
  vt_shader_->uniform("model_view_matrix", scm::math::mat4f(model_view_matrix));

  vt_shader_->uniform("physical_texture_dim", vt_.physical_texture_tile_size_);
  vt_shader_->uniform("max_level", max_depth_level);
  vt_shader_->uniform("tile_size", scm::math::vec2((uint32_t)vt::VTConfig::get_instance().get_size_tile()));
  vt_shader_->uniform("tile_padding", scm::math::vec2((uint32_t)vt::VTConfig::get_instance().get_size_padding()));

  vt_shader_->uniform("enable_hierarchy", false);
  vt_shader_->uniform("toggle_visualization", false);

  for (uint32_t i = 0; i < vt_.index_texture_hierarchy_.size(); ++i) {
    std::string texture_string = "hierarchical_idx_textures";
    vt_shader_->uniform(texture_string, i, int((i)));
  }

  vt_shader_->uniform("physical_texture_array", 17);

  //apply cut update
  vt::CutDatabase *cut_db = &vt::CutDatabase::get_instance();
  for (vt::cut_map_entry_type cut_entry : (*cut_db->get_cut_map())) {
  	vt::Cut *cut = cut_db->start_reading_cut(cut_entry.first);
  	if (!cut->is_drawn()) {
      cut_db->stop_reading_cut(cut_entry.first);
      continue;
    }
    std::set<uint16_t> updated_levels;
    if (cut->get_front()->get_mem_slots_updated().size() > 0) {
      std::cout << "num updated: " << cut->get_front()->get_mem_slots_updated().size() << std::endl;
      for(auto position_slot_updated : cut->get_front()->get_mem_slots_updated()) {
        const vt::mem_slot_type *mem_slot_updated = cut_db->read_mem_slot_at(position_slot_updated.second);
        if (mem_slot_updated == nullptr || !mem_slot_updated->updated || !mem_slot_updated->locked || mem_slot_updated->pointer == nullptr) {
      	  throw std::runtime_error("updated mem slot inconsistency");
        }
        updated_levels.insert(vt::QuadTree::get_depth_of_node(mem_slot_updated->tile_id));

        //update physical texture
        size_t slots_per_texture = vt::VTConfig::get_instance().get_phys_tex_tile_width() * vt::VTConfig::get_instance().get_phys_tex_tile_width();
        size_t layer = mem_slot_updated->position / slots_per_texture;
        size_t rel_slot_position = mem_slot_updated->position - layer * slots_per_texture;
        size_t x_tile = rel_slot_position % vt::VTConfig::get_instance().get_phys_tex_tile_width();
        size_t y_tile = rel_slot_position / vt::VTConfig::get_instance().get_phys_tex_tile_width();

        scm::math::vec3ui origin = scm::math::vec3ui((uint32_t)x_tile * vt::VTConfig::get_instance().get_size_tile(), (uint32_t)y_tile * vt::VTConfig::get_instance().get_size_tile(), (uint32_t)layer);
        scm::math::vec3ui dimensions = scm::math::vec3ui(vt::VTConfig::get_instance().get_size_tile(), vt::VTConfig::get_instance().get_size_tile(), 1);

        context_->update_sub_texture(vt_.physical_texture_, scm::gl::texture_region(origin, dimensions), 0, get_vt_tex_format(), mem_slot_updated->pointer);
      }
    }

    if (cut->get_front()->get_mem_slots_cleared().size() > 0) {
      for (auto position_slot_cleared : cut->get_front()->get_mem_slots_cleared()) {
        const vt::mem_slot_type *mem_slot_cleared = cut_db->read_mem_slot_at(position_slot_cleared.second);
        if (mem_slot_cleared == nullptr) {
      	  throw std::runtime_error("updated mem slot inconsistency");
        }
        updated_levels.insert(vt::QuadTree::get_depth_of_node(position_slot_cleared.first));
      }
    }

    //update index texture hierarchy
    if (updated_levels.size() > 0) {
      for (uint16_t updated_level : updated_levels) {
        uint32_t size_index_texture = (uint32_t)vt::QuadTree::get_tiles_per_row(updated_level);
        scm::math::vec3ui origin = scm::math::vec3ui(0, 0, 0);
        scm::math::vec3ui dimensions = scm::math::vec3ui(size_index_texture, size_index_texture, 1);
      
        context_->update_sub_texture(
      	  vt_.index_texture_hierarchy_.at(updated_level), 
      	  scm::gl::texture_region(origin, dimensions), 
      	  0, 
      	  scm::gl::FORMAT_RGBA_8UI, 
      	  cut->get_front()->get_index(updated_level));
      }
    }

    cut_db->stop_reading_cut(cut_entry.first);

  }

  //context_->sync();


  //bind resources
  for (uint16_t i = 0; i < vt_.index_texture_hierarchy_.size(); ++i) {
    context_->bind_texture(vt_.index_texture_hierarchy_.at(i), filter_nearest_, i);
  }
  context_->bind_texture(vt_.physical_texture_, filter_linear_, 17);
  
  context_->bind_storage_buffer(vt_.feedback_lod_storage_, 0);
  context_->bind_storage_buffer(vt_.feedback_count_storage_, 1);

  context_->apply();


  context_->bind_vertex_array(mesh_.array_);
  context_->apply();

  context_->draw_arrays(scm::gl::PRIMITIVE_TRIANGLE_LIST, 0, mesh_.num_primitives_);
  //std::cout << mesh_.num_primitives_ << std::endl;

  context_->sync();

  //collect feedback
  int32_t* feedback_lod = (int32_t *)context_->map_buffer(vt_.feedback_lod_storage_, scm::gl::ACCESS_READ_WRITE);
  memcpy(&vt_.feedback_lod_cpu_buffer_[0], feedback_lod, vt_.size_feedback_ * scm::gl::size_of_format(scm::gl::FORMAT_R_32I));
  //memset(&vt_.feedback_lod_cpu_buffer_[0], 0, vt_.size_feedback_ * scm::gl::size_of_format(scm::gl::FORMAT_R_32I));
  //context_->sync();
  context_->unmap_buffer(vt_.feedback_lod_storage_);
  context_->clear_buffer_data(vt_.feedback_lod_storage_, scm::gl::FORMAT_R_32I, nullptr);

  uint32_t* feedback_count = (uint32_t*)context_->map_buffer(vt_.feedback_count_storage_, scm::gl::ACCESS_READ_ONLY);
  memcpy(&vt_.feedback_count_cpu_buffer_[0], feedback_count, vt_.size_feedback_ * scm::gl::size_of_format(scm::gl::FORMAT_R_32UI));
  //context_->sync();
  context_->unmap_buffer(vt_.feedback_count_storage_);
  context_->clear_buffer_data(vt_.feedback_count_storage_, scm::gl::FORMAT_R_32UI, nullptr);

  vt_.cut_update_->feedback(&vt_.feedback_lod_cpu_buffer_[0], &vt_.feedback_count_cpu_buffer_[0]);



  rendering_ = false;
  glutSwapBuffers();

  
}

void create_framebuffer() {

  fbo_ = device_->create_frame_buffer();
  fbo_color_buffer_ = device_->create_texture_2d(scm::math::vec2ui(window_width_, window_height_), scm::gl::FORMAT_RGBA_32F , 1, 1, 1);
  fbo_depth_buffer_ = device_->create_texture_2d(scm::math::vec2ui(window_width_, window_height_), scm::gl::FORMAT_D24, 1, 1, 1);
  fbo_->attach_color_buffer(0, fbo_color_buffer_);
  fbo_->attach_depth_stencil_buffer(fbo_depth_buffer_);

}

void glut_resize(int32_t w, int32_t h) {
  window_width_ = w;
  window_height_ = h;

  context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(window_width_, window_height_)));
  camera_->set_projection_matrix(30, float(window_width_)/float(window_height_),  0.01f, 100.f);

  create_framebuffer();
}



void glut_keyboard(unsigned char key, int32_t x, int32_t y) {
  switch (key) {
    case 27:
      exit(0);
      break;

    default: break;

  }
}


void glut_motion(int32_t x, int32_t y) {

  input_.prev_mouse_ = input_.mouse_;
  input_.mouse_ = scm::math::vec2i(x, y);
  
  camera_->update_trackball(x, y, window_width_, window_height_, input_.mouse_state_);

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

  input_.trackball_x_ = 2.f * float(x - (window_width_/2))/float(window_width_) ;
  input_.trackball_y_ = 2.f * float(window_height_ - y - (window_height_/2))/float(window_height_);
  
  camera_->update_trackball_mouse_pos(input_.trackball_x_, input_.trackball_y_);

}


void glut_idle() {
  glutPostRedisplay();
}


//load an .obj file and return all vertices, normals and coords interleaved
uint32_t load_obj(const std::string& _file, std::vector<vertex>& _vertices) {
  std::vector<float> v;
  std::vector<uint32_t> vindices;
  std::vector<float> n;
  std::vector<uint32_t> nindices;
  std::vector<float> t;
  std::vector<uint32_t> tindices;

  uint32_t num_tris = 0;

  FILE* file = fopen(_file.c_str(), "r");

  if (0 != file) {
  
    while (true) {
      char line[128];
      int32_t l = fscanf(file, "%s", line);
			
      if (l == EOF) break;
      if (strcmp(line, "v") == 0) {
        float vx, vy, vz;
        fscanf(file, "%f %f %f\n", &vx, &vy, &vz);
        v.insert(v.end(), {vx, vy, vz});
      }
      else if (strcmp(line, "vn") == 0) {
        float nx, ny, nz;
        fscanf(file, "%f %f %f\n", &nx, &ny, &nz);
        n.insert(n.end(), {nx, ny, nz});
      }
      else if (strcmp(line, "vt") == 0) {
        float tx, ty;
        fscanf(file, "%f %f %f\n", &tx, &ty);
        t.insert(t.end(), {tx, 1.0-ty});
      }
      else if (strcmp(line, "f") == 0) {
        std::string vertex1, vertex2, vertex3;
        uint32_t index[3];
        uint32_t coord[3];
        uint32_t normal[3];
        fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", 
          &index[0], &coord[0], &normal[0], 
          &index[1], &coord[1], &normal[1], 
          &index[2], &coord[2], &normal[2]);
				
        vindices.insert(vindices.end(), {index[0], index[1], index[2]});
        tindices.insert(tindices.end(), {coord[0], coord[1], coord[2]});
        nindices.insert(nindices.end(), {normal[0], normal[1], normal[2]});
      }
    }

    fclose(file);

    std::cout << "positions: " << vindices.size() << std::endl;
    std::cout << "normals: " << nindices.size() << std::endl;
    std::cout << "coords: " << tindices.size() << std::endl;

    _vertices.resize(nindices.size());

    for (uint32_t i = 0; i < nindices.size(); i++) {
      vertex vertex;

      vertex.position_ = scm::math::vec3f(
      	v[3*(vindices[i]-1)], v[3*(vindices[i]-1)+1], v[3*(vindices[i]-1)+2]);

      vertex.normal_ = scm::math::vec3f(
      	n[3*(nindices[i]-1)], n[3*(nindices[i]-1)+1], n[3*(nindices[i]-1)+2]);

      vertex.coords_ = scm::math::vec2f(
      	t[2*(tindices[i]-1)], t[2*(tindices[i]-1)+1]);

      _vertices[i] = vertex;
    }

    num_tris = _vertices.size()/3;

  }
  else {
    std::cout << "failed to open file: " << _file << std::endl;
    exit(1);
  }

  return num_tris;
}



std::string const strip_whitespace(std::string const& in_string) {
  return boost::regex_replace(in_string, boost::regex("^ +| +$|( ) +"), "$1");
}

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

  std::string obj_file = "";
  std::string atlas_file = "";
  bool terminate = false;
  if (cmd_option_exists(argv, argv+argc, "-f")) {
    obj_file = get_cmd_option(argv, argv+argc, "-f");
  }
  else {
    terminate = true;
  }
  
  if (cmd_option_exists(argv, argv+argc, "-t")) {
    atlas_file = get_cmd_option(argv, argv+argc, "-t");
  }
  else {
    terminate = true;
  }

  if (terminate) {
    std::cout << "Usage: " << argv[0] << "<flags>\n" <<
      "INFO: " << argv[0] << "\n" <<
      "\t-f: select .obj input file\n" << 
      "\t-t: select .atlas input file\n" <<
      "\n";
    return 0;
  }

  std::vector<vertex> vertices;
  uint32_t num_tris = load_obj(obj_file, vertices);

  std::cout << num_tris << " triangles" << std::endl;
  

  //putenv((char *)"__GL_SYNC_TO_VBLANK=0");


  glutInit(&argc, argv);
  glutInitContextVersion(4, 4);
  glutInitContextProfile(GLUT_CORE_PROFILE);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
  //glewExperimental = GL_TRUE;
  //glewInit();
  glutInitWindowSize(window_width_, window_height_);
  glutInitWindowPosition(64, 64);
  glutCreateWindow(argv[0]);
  glutSetWindowTitle("lamure_vt_obj_renderer");
  
  glutDisplayFunc(glut_display);
  glutReshapeFunc(glut_resize);
  glutKeyboardFunc(glut_keyboard);
  glutMotionFunc(glut_motion);
  glutMouseFunc(glut_mouse);
  glutIdleFunc(glut_idle);

  device_.reset(new scm::gl::render_device());


  try
  {
  	std::string shader_root_path = LAMURE_SHADERS_DIR;

    std::string shader_vs_source;
    std::string shader_fs_source;
    std::string vt_shader_vs_source;
    std::string vt_shader_fs_source;

    if (!read_shader(shader_root_path + "/trimesh.glslv", shader_vs_source)
      || !read_shader(shader_root_path + "/trimesh.glslf", shader_fs_source)
      || !read_shader(shader_root_path + "/vt/virtual_texturing.glslv", vt_shader_vs_source)
      || !read_shader(shader_root_path + "/vt/virtual_texturing_hierarchical.glslf", vt_shader_fs_source)) {
      std::cout << "error reading shader files " << std::endl;
      return 1;
    }

    shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, shader_vs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, shader_fs_source)));
    if (!shader_) {
      std::cout << "error creating shader program" << std::endl;
      return 1;
    }

    vt_shader_ = device_->create_program(
      boost::assign::list_of
        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vt_shader_vs_source))
        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, vt_shader_fs_source)));
    if (!vt_shader_) {
      std::cout << "error creating vt shader program" << std::endl;
      return 1;
    }
  }
  catch (std::exception& e) {
    std::cout << e.what() << std::endl;
  }

  mesh_.num_primitives_ = vertices.size();
  mesh_.buffer_.reset();
  mesh_.array_.reset();
  mesh_.buffer_ = device_->create_buffer(
    scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, sizeof(vertex) * mesh_.num_primitives_, &vertices[0]);
  mesh_.array_ = device_->create_vertex_array(scm::gl::vertex_format
    (0, 0, scm::gl::TYPE_VEC3F, sizeof(vertex))
    (0, 1, scm::gl::TYPE_VEC2F, sizeof(vertex))
    (0, 2, scm::gl::TYPE_VEC3F, sizeof(vertex)),
    boost::assign::list_of(mesh_.buffer_));


  create_framebuffer();
  color_no_blending_state_ = device_->create_blend_state(false);
  depth_state_less_ = device_->create_depth_stencil_state(true, true, scm::gl::COMPARISON_LESS);
  no_backface_culling_rasterizer_state_ = device_->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, false, false, 0.0, false, false);
  culling_rasterizer_state_ = device_->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_BACK, scm::gl::ORIENT_CCW, true);
  filter_linear_ = device_->create_sampler_state(scm::gl::FILTER_MIN_MAG_LINEAR, scm::gl::WRAP_CLAMP_TO_EDGE);
  filter_nearest_ = device_->create_sampler_state(scm::gl::FILTER_MIN_MAG_NEAREST, scm::gl::WRAP_CLAMP_TO_EDGE);


  context_ = device_->main_context();


  vt::VTConfig::CONFIG_PATH = atlas_file.substr(0, atlas_file.size()-5) + "ini";

  std::cout << vt::VTConfig::CONFIG_PATH  << std::endl;

  vt::VTConfig::get_instance().define_size_physical_texture(128, 8192);
  vt_.texture_id_ = vt::CutDatabase::get_instance().register_dataset(atlas_file);
  vt_.view_id_ = vt::CutDatabase::get_instance().register_view();
  vt_.context_id_ = vt::CutDatabase::get_instance().register_context();
  vt_.cut_id_ = vt::CutDatabase::get_instance().register_cut(vt_.texture_id_, vt_.view_id_, vt_.context_id_);
  vt_.cut_update_ = &vt::CutUpdate::get_instance();
  vt_.cut_update_->start();

  //init index texture hierarchy
  uint16_t depth = (uint16_t)((*vt::CutDatabase::get_instance().get_cut_map())[vt_.cut_id_]->get_atlas()->getDepth());
  uint16_t level = 0;
  while(level++ < depth) {
    uint32_t size_index_texture = (uint32_t)vt::QuadTree::get_tiles_per_row(level);
    auto index_texture_level_ptr = device_->create_texture_2d(scm::math::vec2ui(size_index_texture, size_index_texture), scm::gl::FORMAT_RGBA_8UI);
    device_->main_context()->clear_image_data(index_texture_level_ptr, 0, scm::gl::FORMAT_RGBA_8UI, 0);
    vt_.index_texture_hierarchy_.emplace_back(index_texture_level_ptr);
  }

  //init physical texture
  vt_.physical_texture_tile_size_ = scm::math::vec2ui(vt::VTConfig::get_instance().get_phys_tex_tile_width(), vt::VTConfig::get_instance().get_phys_tex_tile_width());
  vt_.physical_texture_size_ = scm::math::vec2ui(vt::VTConfig::get_instance().get_phys_tex_px_width(), vt::VTConfig::get_instance().get_phys_tex_px_width());
  vt_.physical_texture_ = device_->create_texture_2d(vt_.physical_texture_size_, get_vt_tex_format(), 1, vt::VTConfig::get_instance().get_phys_tex_layers() + 1);

  //init feedback
  vt_.size_feedback_ = vt::VTConfig::get_instance().get_phys_tex_tile_width() * vt::VTConfig::get_instance().get_phys_tex_tile_width() * vt::VTConfig::get_instance().get_phys_tex_layers();
  vt_.feedback_lod_storage_ = device_->create_buffer(scm::gl::BIND_STORAGE_BUFFER, scm::gl::USAGE_STREAM_COPY, vt_.size_feedback_ * scm::gl::size_of_format(scm::gl::FORMAT_R_32I));
  vt_.feedback_count_storage_ = device_->create_buffer(scm::gl::BIND_STORAGE_BUFFER, scm::gl::USAGE_STREAM_COPY, vt_.size_feedback_ * scm::gl::size_of_format(scm::gl::FORMAT_R_32UI));
  vt_.feedback_lod_cpu_buffer_.resize(vt_.size_feedback_, 0);
  vt_.feedback_count_cpu_buffer_.resize(vt_.size_feedback_, 0);


  glutShowWindow();

  scm::math::vec3f center(0.0);
  for (const auto& v : vertices) {
  	center += v.position_;
  }
  center /= (float)vertices.size();
  std::cout << center << std::endl;
  std::cout << vertices[0].position_ << std::endl;

  camera_ = new lamure::ren::camera(0, 
      scm::math::make_look_at_matrix(center+scm::math::vec3f(0.f, 0.1f, -0.01f), center, scm::math::vec3f(0.f, 1.f, 0.f)), 
      5.f, false, false);
  camera_->set_dolly_sens_(20.f);
  
  camera_->set_projection_matrix(30.f, float(window_width_)/float(window_height_), 0.01f, 100.f);

  glutMainLoop();

  return 0;
}
