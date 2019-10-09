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


//schism
#include <scm/core.h>
#include <scm/core/io/tools.h>
#include <scm/gl_util/primitives/primitives_fwd.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/font/font_face.h>
#include <scm/gl_util/font/text.h>
#include <scm/gl_util/font/text_renderer.h>
#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/primitives/wavefront_obj.h>


#include <GL/freeglut.h>

//boost
#include <boost/assign/list_of.hpp>
#include <boost/regex.hpp>

#include "Utils.h"

using namespace std;


int32_t window_width_ = 1920;
int32_t window_height_ = 1080;

std::string obj_file_;

input   input_;

lamure::ren::camera *camera_ = nullptr;

scm::shared_ptr<scm::gl::render_device> device_;
scm::shared_ptr<scm::gl::render_context> context_;

scm::gl::program_ptr shader_;

scm::gl::blend_state_ptr         color_no_blending_state_;
scm::gl::rasterizer_state_ptr    culling_rasterizer_state_;
scm::gl::rasterizer_state_ptr    no_backface_culling_rasterizer_state_;
scm::gl::depth_stencil_state_ptr depth_state_less_;

scm::gl::sampler_state_ptr filter_linear_;
scm::gl::sampler_state_ptr filter_nearest_;

scm::shared_ptr<scm::gl::wavefront_obj_geometry> obj_;


void check_cmd_options(int argc, char *argv[]) {
    bool terminate = false;

    if (Utils::cmd_option_exists(argv, argv + argc, "-f")) {
        obj_file_ = Utils::get_cmd_option(argv, argv + argc, "-f");
    } else {
        terminate = true;
    }

    if (terminate) {
        std::cout << "Usage: " << argv[0] << "<flags>\n" <<
                  "INFO: " << argv[0] << "\n" <<
                  "\t-f: select .obj input file\n" <<
                  "\n";
        std::exit(0);
    }
}

void clear_buffer() {
    context_->set_default_frame_buffer();
    context_->clear_default_color_buffer(scm::gl::FRAMEBUFFER_BACK, scm::math::vec4f(.2f, .2f, .2f, 1.0f));
    context_->clear_default_depth_stencil_buffer();
    context_->apply();
}

void glut_display() {


    clear_buffer();

    auto view_matrix = camera_->get_view_matrix();

    scm::math::mat4f projection_matrix = scm::math::mat4f::identity();
    perspective_matrix(projection_matrix, 20.f, float(window_width_) / float(window_height_), 0.01f, 1000.0f);

    scm::math::mat4f model_matrix = scm::math::mat4f::identity();
    scm::math::mat4f model_view_matrix = view_matrix * model_matrix;

    shader_->uniform("mvp_matrix", projection_matrix*model_view_matrix);
    shader_->uniform("model_view_matrix", model_view_matrix);

    context_->set_viewport(
            scm::gl::viewport(scm::math::vec2ui(0, 0), 1 * scm::math::vec2ui(window_width_, window_height_)));

    context_->set_depth_stencil_state(depth_state_less_);
    context_->set_rasterizer_state(no_backface_culling_rasterizer_state_);
    context_->set_blend_state(color_no_blending_state_);

    context_->bind_program(shader_);
    context_->sync();
    context_->apply();

    obj_->draw(context_, scm::gl::geometry::MODE_SOLID);

    context_->sync();

    glutSwapBuffers();
}

void glut_resize(int32_t w, int32_t h) {
    /*
    window_width_ = w;
    window_height_ = h;

    context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(window_width_, window_height_)));
    camera_->set_projection_matrix(30, float(window_width_) / float(window_height_), 0.01f, 100.f);

    create_framebuffer();*/
}

#define ESC 27
#define SPACEBAR 32

void glut_keyboard(unsigned char key, int32_t x, int32_t y) {
    switch (key) {
        case ESC:
            exit(0);

        case SPACEBAR:
            break;

        default:
            break;
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
        default:
            break;
    }

    input_.prev_mouse_ = input_.mouse_;
    input_.mouse_ = scm::math::vec2i(x, y);

    input_.trackball_x_ = 2.f * float(x - (window_width_ / 2)) / float(window_width_);
    input_.trackball_y_ = 2.f * float(window_height_ - y - (window_height_ / 2)) / float(window_height_);

    camera_->update_trackball_mouse_pos(input_.trackball_x_, input_.trackball_y_);
}

void glut_idle() {
    glutPostRedisplay();
}

void init_glut(int &argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitContextVersion(4, 4);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);

    glutInitWindowSize(window_width_, window_height_);
    glutInitWindowPosition(64, 64);
    glutCreateWindow(argv[0]);
    glutSetWindowTitle("lamure_obj");

    glutDisplayFunc(glut_display);
    glutReshapeFunc(glut_resize);
    glutKeyboardFunc(glut_keyboard);
    glutMotionFunc(glut_motion);
    glutMouseFunc(glut_mouse);
    glutIdleFunc(glut_idle);
}

void init_shader() {
    std::string vertex_source, fragment_source;

    if (!scm::io::read_text_file(
            std::string(LAMURE_SHADERS_DIR) + "/g_trimesh.glslv", vertex_source) ||
        !scm::io::read_text_file(
                std::string(LAMURE_SHADERS_DIR) + "/g_trimesh.glslf", fragment_source)) {
        scm::err() << "error reading shader files" << scm::log::end;
        throw std::runtime_error("Error reading shader files");
    }

    //error here
    obj_.reset(new scm::gl::wavefront_obj_geometry(device_, obj_file_));


    shader_ = device_->create_program(boost::assign::list_of(
            device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vertex_source))(
            device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, fragment_source)));

    if (!shader_) {
        scm::err() << "error creating shader program" << scm::log::end;
        throw std::runtime_error("Error creating shader program");
    }
}

void init_render_states() {
    depth_state_less_        = device_->create_depth_stencil_state(true, true, scm::gl::COMPARISON_LESS);
    color_no_blending_state_ = device_->create_blend_state(true, scm::gl::FUNC_SRC_COLOR,
                                                           scm::gl::FUNC_ONE_MINUS_SRC_ALPHA, scm::gl::FUNC_SRC_ALPHA,
                                                           scm::gl::FUNC_ONE_MINUS_SRC_ALPHA);

    culling_rasterizer_state_             = device_->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_BACK,
                                                                             scm::gl::ORIENT_CCW, true);
    no_backface_culling_rasterizer_state_ = device_->create_rasterizer_state(scm::gl::FILL_SOLID, scm::gl::CULL_NONE,
                                                                             scm::gl::ORIENT_CCW, true);

    filter_nearest_ = device_->create_sampler_state(scm::gl::FILTER_MIN_MAG_NEAREST, scm::gl::WRAP_CLAMP_TO_EDGE);
    filter_linear_  = device_->create_sampler_state(scm::gl::FILTER_MIN_MAG_LINEAR, scm::gl::WRAP_CLAMP_TO_EDGE);
}

void init_camera(const vector<vertex> &vertices) {
    scm::math::vec3f center(0.0);
    for (const auto &v : vertices) {
        center += v.position_;
    }
    center /= (float) vertices.size();
    cout << center << endl;
    cout << vertices[0].position_ << endl;

    camera_ = new lamure::ren::camera(0,
                                      make_look_at_matrix(center + scm::math::vec3f(0.f, 0.1f, -0.01f), center,
                                                          scm::math::vec3f(0.f, 1.f, 0.f)),
                                      5.f, false, false);


    camera_->set_dolly_sens_(20.f);
    camera_->set_projection_matrix(30.f, float(window_width_) / float(window_height_), 0.01f, 100.f);

}


int32_t main(int argc, char *argv[]) {
    check_cmd_options(argc, argv);

    std::vector<vertex> vertices;
    uint32_t num_tris = Utils::load_obj(obj_file_, vertices);



    init_glut(argc, argv);


    device_.reset(new scm::gl::render_device());
    context_ = device_->main_context();


    init_shader();


    init_render_states();



    init_camera(vertices);

    std::cout << num_tris << " triangles" << std::endl;


    glutShowWindow();
    glutMainLoop();

    return 0;
}