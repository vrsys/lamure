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

//schism
#include <scm/core/math.h>
#include <scm/core.h>
#include <scm/core/io/tools.h>
#include <scm/core/pointer_types.h>
#include <scm/gl_core/gl_core_fwd.h>
#include <scm/gl_util/primitives/primitives_fwd.h>
#include <scm/core/platform/platform.h>
#include <scm/core/utilities/platform_warning_disable.h>
#include <scm/gl_core/render_device/opengl/gl_core.h>
#include <scm/gl_util/primitives/quad.h>

#include <GL/freeglut.h>

//boost
#include <boost/assign/list_of.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>

//fwd
void initialize_glut(int argc, char** argv, uint32_t width, uint32_t height);
void glut_display();
void glut_resize(int w, int h);
void glut_mousefunc(int button, int state, int x, int y);
void glut_mousemotion(int x, int y);
void glut_idle();
void glut_keyboard(unsigned char key, int x, int y);
void glut_keyboard_release(unsigned char key, int x, int y);
void glut_specialfunc(int key, int x, int y);
void glut_specialfunc_release(int key, int x, int y);
void glut_close();

uint32_t update_ms_ = 16;
bool rendering_ = false;
int32_t window_width_ = 1280;
int32_t window_height_ = 720;
float frame_div = 1;
uint64_t frame_ = 0;
bool fast_travel_ = false;

float near_plane_ = 0.01f;
float far_plane_ = 60.f;

float trackball_x_ = 0.f;
float trackball_y_ = 0.f;
float dolly_sens_ = 10.f;
float fov_ = 30.f;

std::vector<std::string> input_files_;

std::vector<scm::math::mat4d> model_transformations_;
int32_t num_models_ = 0;
int32_t selected_model_ = -1;

float point_size_ = 1.0f;
float error_threshold_ = 2.0f;

lamure::ren::camera::mouse_state mouse_state_;

lamure::ren::camera* camera_ = nullptr;

scm::shared_ptr<scm::gl::render_device> device_;
scm::shared_ptr<scm::gl::render_context> context_;
scm::gl::program_ptr vis_xyz_shader_;
scm::gl::frame_buffer_ptr fbo_;
scm::gl::rasterizer_state_ptr no_backface_culling_rasterizer_state_;
scm::gl::texture_2d_ptr color_buffer_;
scm::gl::texture_2d_ptr depth_buffer_;
scm::gl::sampler_state_ptr state_linear_;
scm::gl::program_ptr quad_shader_;
scm::shared_ptr<scm::gl::quad_geometry> screen_quad_;

lamure::ren::Data_Provenance data_provenance_;

struct settings {
  int32_t width_;
  int32_t height_;
  int32_t vram_;
  int32_t ram_;
  int32_t upload_;
  int32_t prov_;
  std::string json_;
  std::string pvs_;
  std::vector<std::string> models_;

};

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
        std::istringstream line_ss(line);
        
        auto colon = line.find_first_of(':');
        if (colon == std::string::npos) {
          //std::cout << "lod: " << line << std::endl;
          settings.models_.push_back(line);

        }
        else {
          std::string key = line.substr(0, colon);
          key.erase(std::remove(key.begin(), key.end(), ' '), key.end());
          std::string value = line.substr(colon+1);
          value.erase(std::remove(value.begin(), value.end(), ' '), value.end());
          if (key == "width") {
            settings.width_ = std::max(atoi(value.c_str()), 64);
          }
          else if (key == "height") {
            settings.height_ = std::max(atoi(value.c_str()), 64);
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
          else if (key == "prov") {
            settings.prov_ = std::max(atoi(value.c_str()), 0);
          }
          else if (key == "json") {
            settings.json_ = value;
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
  }
  if (settings.models_.empty()) {
    std::cout << "error: no model filename specified" << std::endl;
    exit(-1);
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

static void glut_timer(int e) {
  glutPostRedisplay();
  glutTimerFunc(update_ms_, glut_timer, 1);
}

void glut_display() {
  if (rendering_) {
    return;
  }
  rendering_ = true;

  lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
  lamure::ren::controller* controller = lamure::ren::controller::get_instance();
  lamure::ren::cut_database* cuts = lamure::ren::cut_database::get_instance();

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
    cuts->send_threshold(context_id, m_id, error_threshold_);
    cuts->send_rendered(context_id, m_id);
    
    database->get_model(m_id)->set_transform(scm::math::mat4f(model_transformations_[m_id]));
  }


  lamure::view_t cam_id = controller->deduce_view_id(context_id, camera_->view_id());
  cuts->send_camera(context_id, cam_id, *camera_);

  std::vector<scm::math::vec3d> corner_values = camera_->get_frustum_corners();
  double top_minus_bottom = scm::math::length((corner_values[2]) - (corner_values[0]));
  float height_divided_by_top_minus_bottom = lamure::ren::policy::get_instance()->window_height() / top_minus_bottom;

  cuts->send_height_divided_by_top_minus_bottom(context_id, cam_id, height_divided_by_top_minus_bottom);
 
  if (lamure::ren::policy::get_instance()->size_of_provenance() > 0) {
    controller->dispatch(context_id, device_, data_provenance_);
  }
  else {
    controller->dispatch(context_id, device_); 
  }
  lamure::view_t view_id = controller->deduce_view_id(context_id, camera_->view_id());
 
  context_->set_frame_buffer(fbo_);
  //context_->set_default_frame_buffer();
  
  
  context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(window_width_, window_height_)));
  context_->clear_depth_stencil_buffer(fbo_);
  context_->clear_color_buffer(fbo_, 0, 
    scm::math::vec4f(LAMURE_DEFAULT_COLOR_R, LAMURE_DEFAULT_COLOR_G, LAMURE_DEFAULT_COLOR_B, 1.0f));
  //context_->clear_default_depth_stencil_buffer();
  //context_->clear_default_color_buffer();
  
  
  context_->bind_program(vis_xyz_shader_);
  context_->set_rasterizer_state(no_backface_culling_rasterizer_state_);
  
  vis_xyz_shader_->uniform("near_plane", near_plane_);
  vis_xyz_shader_->uniform("far_plane", far_plane_);
  vis_xyz_shader_->uniform("point_size_factor", point_size_);
  
  if (lamure::ren::policy::get_instance()->size_of_provenance() > 0) {
    context_->bind_vertex_array(
      controller->get_context_memory(context_id, lamure::ren::bvh::primitive_type::POINTCLOUD, device_, data_provenance_));
  }
  else {
   context_->bind_vertex_array(
      controller->get_context_memory(context_id, lamure::ren::bvh::primitive_type::POINTCLOUD, device_)); 
  }
  context_->apply();
  
  for (int32_t model_id = 0; model_id < num_models_; ++model_id) {
    if (selected_model_ != -1) {
      model_id = selected_model_;
    }
    lamure::ren::cut& cut = cuts->get_cut(context_id, view_id, model_id);
    std::vector<lamure::ren::cut::node_slot_aggregate> renderable = cut.complete_set();
    const lamure::ren::bvh* bvh = database->get_model(model_id)->get_bvh();
    if (bvh->get_primitive() != lamure::ren::bvh::primitive_type::POINTCLOUD) {
      continue;
    }
    
    //uniforms per model
    scm::math::mat4d model_matrix = model_transformations_[model_id];
    scm::math::mat4d projection_matrix = scm::math::mat4d(camera_->get_projection_matrix());
    scm::math::mat4d view_matrix = camera_->get_high_precision_view_matrix();
    scm::math::mat4d model_view_matrix = view_matrix * model_matrix;
    scm::math::mat4d model_view_projection_matrix = projection_matrix * model_view_matrix;

    vis_xyz_shader_->uniform("mvp_matrix", scm::math::mat4f(model_view_projection_matrix));
    vis_xyz_shader_->uniform("model_view_matrix", scm::math::mat4f(model_view_matrix));
    vis_xyz_shader_->uniform("inv_mv_matrix", scm::math::mat4f(scm::math::transpose(scm::math::inverse(model_view_matrix))));
    vis_xyz_shader_->uniform("model_radius_scale", 1.f);
    vis_xyz_shader_->uniform("projection_matrix", scm::math::mat4f(projection_matrix));

    vis_xyz_shader_->uniform("heatmap_min", 0.0f);
    vis_xyz_shader_->uniform("heatmap_max", 0.05f);
    vis_xyz_shader_->uniform("heatmap_min_color", scm::math::vec3f(68.f/255.f, 0.f, 84.f/255.f));
    vis_xyz_shader_->uniform("heatmap_max_color", scm::math::vec3f(251.f/255.f, 231.f/255.f, 35.f/255.f));

    
    size_t surfels_per_node = database->get_primitives_per_node();
    std::vector<scm::gl::boxf>const & bounding_box_vector = bvh->get_bounding_boxes();
    
    scm::gl::frustum frustum_by_model = camera_->get_frustum_by_model(scm::math::mat4f(model_transformations_[model_id]));
    
    for(auto const& node_slot_aggregate : renderable) {
      uint32_t node_culling_result = camera_->cull_against_frustum(
        frustum_by_model,
        bounding_box_vector[node_slot_aggregate.node_id_]);
        
      if (node_culling_result != 1) {
        context_->draw_arrays(scm::gl::PRIMITIVE_POINT_LIST,
          (node_slot_aggregate.slot_id_) * (GLsizei)surfels_per_node, surfels_per_node);
      
      }
    }
    if (selected_model_ != -1) {
      break;
    }
  }
  
  
  context_->set_default_frame_buffer();
  context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(window_width_, window_height_)));
  context_->clear_default_depth_stencil_buffer();
  context_->clear_default_color_buffer();
  
  context_->bind_program(quad_shader_);
  
  context_->bind_texture(color_buffer_, state_linear_, 0);
  context_->apply();
  
  screen_quad_->draw(context_);

  rendering_ = false;
  glutSwapBuffers();
  
}


void glut_resize(int32_t w, int32_t h) {
  window_width_ = w;
  window_height_ = h;
  context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(w, h)));

  fbo_ = device_->create_frame_buffer();
  color_buffer_ = device_->create_texture_2d(scm::math::vec2ui(window_width_, window_height_) * 1, scm::gl::FORMAT_RGBA_32F , 1, 1, 1);
  depth_buffer_ = device_->create_texture_2d(scm::math::vec2ui(w, h) * 1, scm::gl::FORMAT_D32F, 1, 1, 1);
  fbo_->attach_color_buffer(0, color_buffer_);
  fbo_->attach_depth_stencil_buffer(depth_buffer_);
  
  lamure::ren::policy* policy = lamure::ren::policy::get_instance();
  policy->set_window_width(w);
  policy->set_window_height(h);
  
  camera_->set_projection_matrix(30.0f, float(w)/float(h),  near_plane_, far_plane_);

}

void glut_keyboard(unsigned char key, int32_t x, int32_t y) {
  switch (key) {
    case 27:
      exit(0);
      break;

    case '.':
      glutFullScreenToggle();
      break;

    case 'u':
      if (point_size_ > 0.1) {
        point_size_ -= 0.1;
      }
      break;

    case 'j':
      if (point_size_ < 6) {
        point_size_ += 0.1;
      }
      break;
      
    
    case 'f':
      std::cout<<"fast travel: ";
      if (fast_travel_) {
        camera_->set_dolly_sens_(0.5f);
        std::cout<<"OFF\n\n";
      }
      else {
        camera_->set_dolly_sens_(20.5f);
        std::cout<<"ON\n\n";
      }
      fast_travel_ = ! fast_travel_;
      break;

    case '0':
      selected_model_ = -1;
      break;
    case '1':
      selected_model_ = std::min(num_models_-1, 0);
      break;
    case '2':
      selected_model_ = std::min(num_models_-1, 1);
      break;
    case '3':
      selected_model_ = std::min(num_models_-1, 2);
      break;
    case '4':
      selected_model_ = std::min(num_models_-1, 3);
      break;
    case '5':
      selected_model_ = std::min(num_models_-1, 4);
      break;
    case '6':
      selected_model_ = std::min(num_models_-1, 5);
      break;
    case '7':
      selected_model_ = std::min(num_models_-1, 6);
      break;
    case '8':
      selected_model_ = std::min(num_models_-1, 7);
      break;
    case '9':
      selected_model_ = std::min(num_models_-1, 8);
      break;
      

    default:
      break;

  }

}


void glut_motion(int32_t x, int32_t y) {

  camera_->update_trackball(x,y, window_width_, window_height_, mouse_state_);

}

void glut_mouse(int32_t button, int32_t state, int32_t x, int32_t y) {

  switch (button) {
    case GLUT_LEFT_BUTTON:
    {
        mouse_state_.lb_down_ = (state == GLUT_DOWN) ? true : false;
    } break;
    case GLUT_MIDDLE_BUTTON:
    {
        mouse_state_.mb_down_ = (state == GLUT_DOWN) ? true : false;
    } break;
    case GLUT_RIGHT_BUTTON:
    {
        mouse_state_.rb_down_ = (state == GLUT_DOWN) ? true : false;
    } break;
  }

  trackball_x_ = 2.f * float(x - (window_width_/2))/float(window_width_) ;
  trackball_y_ = 2.f * float(window_height_ - y - (window_height_/2))/float(window_height_);
  
  camera_->update_trackball_mouse_pos(trackball_x_, trackball_y_);
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

  settings settings{1920, 1080, 2048, 4096, 32, 0, "", "", std::vector<std::string>()};
  load_settings(vis_file, settings);
 
  lamure::ren::policy* policy = lamure::ren::policy::get_instance();
  policy->set_max_upload_budget_in_mb(settings.upload_);
  policy->set_render_budget_in_mb(settings.vram_);
  policy->set_out_of_core_budget_in_mb(settings.ram_);
  window_width_ = settings.width_;
  window_height_ = settings.height_;
  policy->set_window_width(window_width_);
  policy->set_window_height(window_height_);
  policy->set_size_of_provenance(settings.prov_);

  if (policy->size_of_provenance() > 0) {
    data_provenance_ = lamure::ren::Data_Provenance::parse_json(settings.json_);
  }

  lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
  
  float scene_diameter = far_plane_;
  num_models_ = 0;
  for (const auto& input_file : settings.models_) {
    lamure::model_t model_id = database->add_model(input_file, std::to_string(num_models_));
    
    const auto& bb = database->get_model(num_models_)->get_bvh()->get_bounding_boxes()[0];
    scene_diameter = scm::math::max(scm::math::length(bb.max_vertex()-bb.min_vertex()), scene_diameter);
    model_transformations_.push_back(scm::math::mat4d(scm::math::make_translation(database->get_model(num_models_)->get_bvh()->get_translation())));
    
    ++num_models_;
  }
  
  if(settings.pvs_ != "") {
    std::cout << "loading pvs: " << settings.pvs_ << std::endl;
    std::string pvs_grid_file_path = settings.pvs_;
    pvs_grid_file_path.resize(pvs_grid_file_path.length() - 3);
    pvs_grid_file_path = pvs_grid_file_path + "grid";

    lamure::pvs::pvs_database* pvs = lamure::pvs::pvs_database::get_instance();
    pvs->load_pvs_from_file(pvs_grid_file_path, settings.pvs_, false);
  }
 
  glutInit(&argc, argv);
  glutInitContextVersion(4, 4);
  glutInitContextProfile(GLUT_CORE_PROFILE);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
  glutInitWindowSize(window_width_, window_height_);
  glutInitWindowPosition(64, 64);
  glutCreateWindow(argv[0]);
  glutSetWindowTitle("lamure_vis");
  //glewExperimental = GL_TRUE;
  //glewInit();
  //glutHideWindow();
  
  glutDisplayFunc(glut_display);
  glutReshapeFunc(glut_resize);
  glutKeyboardFunc(glut_keyboard);
  glutMotionFunc(glut_motion);
  glutMouseFunc(glut_mouse);

  device_.reset(new scm::gl::render_device());


  context_ = device_->main_context();
  

  std::string lq_one_pass_vs_source;
  std::string lq_one_pass_gs_source;
  std::string lq_one_pass_fs_source;
  std::string quad_shader_fs_source;
  std::string quad_shader_vs_source;
  if (!scm::io::read_text_file("../share/lamure/shaders/vis_xyz.glslv", lq_one_pass_vs_source)
    || !scm::io::read_text_file("../share/lamure/shaders/vis_xyz.glslg", lq_one_pass_gs_source)
    || !scm::io::read_text_file("../share/lamure/shaders/vis_xyz.glslf", lq_one_pass_fs_source)
    || !scm::io::read_text_file("../share/lamure/shaders/vis_quad.glslv", quad_shader_vs_source)
    || !scm::io::read_text_file("../share/lamure/shaders/vis_quad.glslf", quad_shader_fs_source)) {
    std::cout << "error reading shader files" << std::endl;
    return 1; 
  }

  vis_xyz_shader_ = device_->create_program(
    boost::assign::list_of
      (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, lq_one_pass_vs_source))
      (device_->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, lq_one_pass_gs_source))
      (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, lq_one_pass_fs_source)));
  if (!vis_xyz_shader_) {
    std::cout << "error creating shader programs" << std::endl;
    return 1;
  }

  quad_shader_ = device_->create_program(
    boost::assign::list_of
      (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, quad_shader_vs_source))
      (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, quad_shader_fs_source)));
  if (!quad_shader_) {
    std::cout << "error creating shader programs" << std::endl;
    return 1;
  }

  glutShowWindow();
  
  glutTimerFunc(update_ms_, glut_timer, 1);

  fbo_ = device_->create_frame_buffer();
  color_buffer_ = device_->create_texture_2d(scm::math::vec2ui(window_width_, window_height_), scm::gl::FORMAT_RGBA_32F , 1, 1, 1);
  depth_buffer_ = device_->create_texture_2d(scm::math::vec2ui(window_width_, window_height_), scm::gl::FORMAT_D32F, 1, 1, 1);
  fbo_->attach_color_buffer(0, color_buffer_);
  fbo_->attach_depth_stencil_buffer(depth_buffer_);
  
  no_backface_culling_rasterizer_state_ = device_->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE, scm::gl::ORIENT_CCW, false, false, 0.0, false, false);

  auto root_bb = database->get_model(0)->get_bvh()->get_bounding_boxes()[0];
  scm::math::vec3f center = scm::math::mat4f(model_transformations_[0]) * ((root_bb.min_vertex() + root_bb.max_vertex()) / 2.f);

  state_linear_ = device_->create_sampler_state(scm::gl::FILTER_ANISOTROPIC, scm::gl::WRAP_CLAMP_TO_EDGE, 16u);
    

  camera_ = new lamure::ren::camera(0, 
    scm::math::make_look_at_matrix(center+scm::math::vec3f(0.f, 0.1f, -0.01f), center, scm::math::vec3f(0.f, 1.f, 0.f)), 
    scm::math::length(root_bb.max_vertex()-root_bb.min_vertex()), false, false);
  
  
  screen_quad_.reset(new scm::gl::quad_geometry(device_, scm::math::vec2f(-1.0f, -1.0f), scm::math::vec2f(1.0f, 1.0f)));
  
  
  glutMainLoop();

  return 0;


}

