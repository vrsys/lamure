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


int32_t window_width_ = 1280;
int32_t window_height_ = 720;

std::string obj_file_;
std::string atlas_file_;

input   input_;
vt_info vt_;

lamure::ren::camera *camera_ = nullptr;

scm::shared_ptr<scm::gl::render_device> device_;
scm::shared_ptr<scm::gl::render_context> context_;

scm::gl::program_ptr vt_shader_;

scm::gl::frame_buffer_ptr fbo_;
scm::gl::texture_2d_ptr   fbo_color_buffer_;
scm::gl::texture_2d_ptr   fbo_depth_buffer_;

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

    if (Utils::cmd_option_exists(argv, argv + argc, "-t")) {
        atlas_file_ = Utils::get_cmd_option(argv, argv + argc, "-t");
    } else {
        terminate = true;
    }

    if (terminate) {
        std::cout << "Usage: " << argv[0] << "<flags>\n" <<
                  "INFO: " << argv[0] << "\n" <<
                  "\t-f: select .obj input file\n" <<
                  "\t-t: select .atlas input file\n" <<
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

void apply_cut_update() {
    auto *cut_db = &vt::CutDatabase::get_instance();

    for (vt::cut_map_entry_type cut_entry : (*cut_db->get_cut_map())) {
        if(vt::Cut::get_context_id(cut_entry.first) != vt_.context_id_)
        {
            continue;
        }

        vt::Cut *cut = cut_db->start_reading_cut(cut_entry.first);

        if (!cut->is_drawn()) {
            cut_db->stop_reading_cut(cut_entry.first);
            continue;
        }

        std::set<uint16_t> updated_levels;

        for (auto position_slot_updated : cut->get_front()->get_mem_slots_updated()) {
            const vt::mem_slot_type *mem_slot_updated = cut_db->read_mem_slot_at(position_slot_updated.second, vt_.context_id_);

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
                                         Utils::get_tex_format(), mem_slot_updated->pointer);
        }


        for (auto position_slot_cleared : cut->get_front()->get_mem_slots_cleared()) {
            const vt::mem_slot_type *mem_slot_cleared = cut_db->read_mem_slot_at(position_slot_cleared.second, vt_.context_id_);

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

void collect_feedback() {
    int32_t *feedback_lod = (int32_t *) context_->map_buffer(vt_.feedback_lod_storage_, scm::gl::ACCESS_READ_ONLY);
    memcpy(vt_.feedback_lod_cpu_buffer_, feedback_lod, vt_.size_feedback_ * size_of_format(scm::gl::FORMAT_R_32I));
    context_->sync();

    context_->unmap_buffer(vt_.feedback_lod_storage_);
    context_->clear_buffer_data(vt_.feedback_lod_storage_, scm::gl::FORMAT_R_32I, nullptr);

    uint32_t *feedback_count = (uint32_t *) context_->map_buffer(vt_.feedback_count_storage_,
                                                                 scm::gl::ACCESS_READ_ONLY);
    memcpy(vt_.feedback_count_cpu_buffer_, feedback_count, vt_.size_feedback_ * size_of_format(scm::gl::FORMAT_R_32UI));
    context_->sync();

    vt_.cut_update_->feedback(vt_.context_id_, vt_.feedback_lod_cpu_buffer_, vt_.feedback_count_cpu_buffer_);

    context_->unmap_buffer(vt_.feedback_count_storage_);
    context_->clear_buffer_data(vt_.feedback_count_storage_, scm::gl::FORMAT_R_32UI, nullptr);
}

void glut_display() {
    clear_buffer();

    // render
    uint64_t color_cut_id =
            (((uint64_t) vt_.texture_id_) << 32) | ((uint64_t) vt_.view_id_ << 16) | ((uint64_t) vt_.context_id_);
    uint32_t max_depth_level_color =
            (*vt::CutDatabase::get_instance().get_cut_map())[color_cut_id]->get_atlas()->getDepth() - 1;

    auto view_matrix = camera_->get_view_matrix();

    scm::math::mat4f projection_matrix = scm::math::mat4f::identity();
    perspective_matrix(projection_matrix, 20.f, float(window_width_) / float(window_height_), 0.01f, 1000.0f);

    scm::math::mat4f model_matrix = scm::math::mat4f::identity();
    scm::math::mat4f model_view_matrix = view_matrix * model_matrix;

    vt_shader_->uniform("projection_matrix", projection_matrix);
    vt_shader_->uniform("model_view_matrix", model_view_matrix);

    vt_shader_->uniform("physical_texture_dim", vt_.physical_texture_size_);
    vt_shader_->uniform("max_level", max_depth_level_color);
    vt_shader_->uniform("tile_size", scm::math::vec2((uint32_t) vt::VTConfig::get_instance().get_size_tile()));
    vt_shader_->uniform("tile_padding", scm::math::vec2((uint32_t) vt::VTConfig::get_instance().get_size_padding()));

    vt_shader_->uniform("enable_hierarchy", vt_.enable_hierarchy_);
    vt_shader_->uniform("toggle_visualization", vt_.toggle_visualization_);

    for (uint32_t i = 0; i < vt_.index_texture_hierarchy_.size(); ++i) {
        std::string texture_string = "hierarchical_idx_textures";
        vt_shader_->uniform(texture_string, i, int((i)));
    }

    vt_shader_->uniform("physical_texture_array", 17);

    context_->set_viewport(
            scm::gl::viewport(scm::math::vec2ui(0, 0), 1 * scm::math::vec2ui(window_width_, window_height_)));

    context_->set_depth_stencil_state(depth_state_less_);
    context_->set_rasterizer_state(no_backface_culling_rasterizer_state_);
    context_->set_blend_state(color_no_blending_state_);

    context_->bind_program(vt_shader_);
    context_->sync();

    apply_cut_update();

    for (uint16_t i = 0; i < vt_.index_texture_hierarchy_.size(); ++i) {
        context_->bind_texture(vt_.index_texture_hierarchy_.at(i), filter_nearest_, i);
    }

    context_->bind_texture(vt_.physical_texture_, filter_linear_, 17);

    context_->bind_storage_buffer(vt_.feedback_lod_storage_, 0);
    context_->bind_storage_buffer(vt_.feedback_count_storage_, 1);

    context_->apply();

    obj_->draw(context_, scm::gl::geometry::MODE_SOLID);

    context_->sync();

    collect_feedback();

    glutSwapBuffers();
}

void create_framebuffer() {
    fbo_ = device_->create_frame_buffer();
    fbo_color_buffer_ = device_->create_texture_2d(scm::math::vec2ui(window_width_, window_height_),
                                                   scm::gl::FORMAT_RGBA_32F, 1, 1, 1);
    fbo_depth_buffer_ = device_->create_texture_2d(scm::math::vec2ui(window_width_, window_height_),
                                                   scm::gl::FORMAT_D24, 1, 1, 1);
    fbo_->attach_color_buffer(0, fbo_color_buffer_);
    fbo_->attach_depth_stencil_buffer(fbo_depth_buffer_);
}

void glut_resize(int32_t w, int32_t h) {
    window_width_ = w;
    window_height_ = h;

    context_->set_viewport(scm::gl::viewport(scm::math::vec2ui(0, 0), scm::math::vec2ui(window_width_, window_height_)));
    camera_->set_projection_matrix(30, float(window_width_) / float(window_height_), 0.01f, 100.f);

    create_framebuffer();
}

#define ESC 27
#define SPACEBAR 32

void glut_keyboard(unsigned char key, int32_t x, int32_t y) {
    switch (key) {
        case ESC:
            exit(0);

        case SPACEBAR:
            vt_.toggle_visualization_ = (vt_.toggle_visualization_ + 1) % 3;
            break;

        case '1':
            vt_.toggle_visualization_ = 0;
            break;

        case '2':
            vt_.toggle_visualization_ = 1;
            break;

        case '3':
            vt_.toggle_visualization_ = 2;
            break;

        case 'h':
            vt_.enable_hierarchy_ = !vt_.enable_hierarchy_;
            break;

        case 'p':
            vt_.cut_update_->toggle_freeze_dispatch();

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
    glutSetWindowTitle("lamure_vt_obj_renderer");

    glutDisplayFunc(glut_display);
    glutReshapeFunc(glut_resize);
    glutKeyboardFunc(glut_keyboard);
    glutMotionFunc(glut_motion);
    glutMouseFunc(glut_mouse);
    glutIdleFunc(glut_idle);
}

void init_vt_database() {
    vt::VTConfig::CONFIG_PATH = atlas_file_.substr(0, atlas_file_.size() - 5) + "ini";

    vt::VTConfig::get_instance().define_size_physical_texture(128, 8192);
    vt_.texture_id_ = vt::CutDatabase::get_instance().register_dataset(atlas_file_);
    vt_.context_id_ = vt::CutDatabase::get_instance().register_context();
    vt_.view_id_    = vt::CutDatabase::get_instance().register_view();
    vt_.cut_id_     = vt::CutDatabase::get_instance().register_cut(vt_.texture_id_, vt_.view_id_, vt_.context_id_);
    vt_.cut_update_ = &vt::CutUpdate::get_instance();
    vt_.cut_update_->start();
}

void init_shader() {
    std::string fs_vt_color, vs_vt;

    if (!scm::io::read_text_file(
            std::string(LAMURE_SHADERS_DIR) + "/virtual_texturing.glslv", vs_vt) ||
        !scm::io::read_text_file(
                std::string(LAMURE_SHADERS_DIR) + "/virtual_texturing_hierarchical.glslf", fs_vt_color)) {
        scm::err() << "error reading shader files" << scm::log::end;
        throw std::runtime_error("Error reading shader files");
    }

    obj_.reset(new scm::gl::wavefront_obj_geometry(device_, obj_file_));

    vt_shader_ = device_->create_program(boost::assign::list_of(
            device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vs_vt))(
            device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, fs_vt_color)));

    if (!vt_shader_) {
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

    vt_.physical_texture_ = device_->create_texture_2d(physical_texture_size, Utils::get_tex_format(), 1,
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

    std::cout << num_tris << " triangles" << std::endl;

    init_glut(argc, argv);
    init_vt_database();

    device_.reset(new scm::gl::render_device());

    init_shader();
    init_render_states();
    init_vt_system();
    init_camera(vertices);

    glutShowWindow();
    glutMainLoop();

    return 0;
}