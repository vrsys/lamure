// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "VTRenderer.h"
#include "imgui_impl_glfw_gl3.h"
#include "ostream.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <lamure/vt/VTConfig.h>
#include <lamure/vt/ren/CutDatabase.h>
#include <lamure/vt/ren/CutUpdate.h>
#include <queue>
#include <unordered_map>

//#define COLLECT_BENCHMARKS

char* get_cmd_option(char** begin, char** end, const std::string& option)
{
    char** it = std::find(begin, end, option);
    if(it != end && ++it != end)
        return *it;
    return nullptr;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) { return std::find(begin, end, option) != end; }

struct Window
{
    Window()
    {
        _mouse_button_state = MouseButtonState::IDLE;
        _trackball_manipulator.dolly(2.5f);
    }

    unsigned int _width;
    unsigned int _height;

    GLFWwindow* _glfw_window;

    scm::gl::trackball_manipulator _trackball_manipulator;

    float _ref_rot_x;
    float _ref_rot_y;

    float _scale = 0.7f;

    enum Dataset
    {
        COLOR = 0,
        ELEVATION = 1,
    };

    enum Interaction
    {
        DEMO = 0,
        EARTH = 1,
        MOON = 2
    };

    int _vis = 0;
    bool _enable_hierarchical = true;
    Interaction _interaction = DEMO;
    Dataset _dataset = COLOR;

    enum MouseButtonState
    {
        LEFT = 0,
        WHEEL = 1,
        RIGHT = 2,
        IDLE = 3
    };

    MouseButtonState _mouse_button_state;
};

std::list<Window*> _windows;
Window* _current_context = nullptr;
vt::CutUpdate* _cut_update = nullptr;

class EventHandler
{
  public:
    static void on_error(int _err_code, const char* err_msg) { throw std::runtime_error(err_msg); }
    static void on_window_resize(GLFWwindow* glfw_window, int width, int height)
    {
        Window* window = (Window*)glfwGetWindowUserPointer(glfw_window);
        window->_height = (uint32_t)height;
        window->_width = (uint32_t)width;
    }
    static void on_window_key_press(GLFWwindow* glfw_window, int key, int scancode, int action, int mods)
    {
        Window* window = (Window*)glfwGetWindowUserPointer(glfw_window);

        if(action == GLFW_PRESS)
        {
            switch(key)
            {
            case GLFW_KEY_ESCAPE:
                std::cout << "should close" << std::endl;
                glfwSetWindowShouldClose(glfw_window, GL_TRUE);
                break;
            case GLFW_KEY_P:
                std::cout << "toggle cut freeze" << std::endl;
                _cut_update->toggle_freeze_dispatch();
                break;
            case GLFW_KEY_0:
                window->_vis = 0;
                break;
            case GLFW_KEY_1:
                window->_vis = 1;
                break;
            case GLFW_KEY_2:
                window->_vis = 2;
                break;
            case GLFW_KEY_E:
                window->_interaction = Window::Interaction::EARTH;
                break;
            case GLFW_KEY_M:
                window->_interaction = Window::Interaction::MOON;
                break;
            case GLFW_KEY_D:
                window->_interaction = Window::Interaction::DEMO;
                break;
            case GLFW_KEY_H:
                window->_enable_hierarchical = !window->_enable_hierarchical;
                break;
            case GLFW_KEY_C:
                window->_dataset = window->_dataset == Window::Dataset::COLOR ? Window::Dataset::ELEVATION : Window::Dataset::COLOR;
                break;
            }
        }

#ifndef NDEBUG
        ImGui_ImplGlfwGL3_KeyCallback(glfw_window, key, scancode, action, mods);
#endif
    }
    static void on_window_char(GLFWwindow* glfw_window, unsigned int codepoint)
    {
        Window* window = (Window*)glfwGetWindowUserPointer(glfw_window);
#ifndef NDEBUG
        ImGui_ImplGlfwGL3_CharCallback(glfw_window, codepoint);
#endif
    }
    static void on_window_button_press(GLFWwindow* glfw_window, int button, int action, int mods)
    {
        Window* window = (Window*)glfwGetWindowUserPointer(glfw_window);
        if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        {
            window->_mouse_button_state = Window::MouseButtonState::LEFT;
        }
        else if(button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
        {
            window->_mouse_button_state = Window::MouseButtonState::WHEEL;
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        {
            window->_mouse_button_state = Window::MouseButtonState::RIGHT;
        }
        else
        {
            window->_mouse_button_state = Window::MouseButtonState::IDLE;
        }

#ifndef NDEBUG
        ImGui_ImplGlfwGL3_MouseButtonCallback(glfw_window, button, action, mods);
#endif
    }
    static void on_window_move_cursor(GLFWwindow* glfw_window, double xpos, double ypos)
    {
        Window* window = (Window*)glfwGetWindowUserPointer(glfw_window);

        float x = ((float)xpos / window->_width - 0.5f) * 6.28f;
        float y = ((float)ypos / window->_height - 0.5f) * 6.28f;

        switch(window->_mouse_button_state)
        {
        case Window::MouseButtonState::LEFT:
        {
            window->_trackball_manipulator.rotation(window->_ref_rot_x, 0, x, 0);
        }
        break;
        case Window::MouseButtonState::RIGHT:
        {
            window->_trackball_manipulator.rotation(0, window->_ref_rot_y, 0, y);
        }
        break;
        default:
            break;
        }

        window->_ref_rot_x = x;
        window->_ref_rot_y = y;
    }
    static void on_window_scroll(GLFWwindow* glfw_window, double xoffset, double yoffset)
    {
        Window* window = (Window*)glfwGetWindowUserPointer(glfw_window);
        window->_scale = std::min(window->_scale + (float)yoffset * 0.01f, 2.3f);

#ifndef NDEBUG
        ImGui_ImplGlfwGL3_ScrollCallback(glfw_window, xoffset, yoffset);
#endif
    }
    static void on_window_enter(GLFWwindow* glfw_window, int entered)
    {
        Window* window = (Window*)glfwGetWindowUserPointer(glfw_window);
        // TODO
    }
};

void make_context_current(Window* _window)
{
    if(_window != nullptr)
    {
        glfwMakeContextCurrent(_window->_glfw_window);
        _current_context = _window;
    }
}

Window* create_window(unsigned int width, unsigned int height, const std::string& title, GLFWmonitor* monitor, Window* share)
{
    Window* previous_context = _current_context;

    Window* new_window = new Window();

    new_window->_glfw_window = nullptr;
    new_window->_width = width;
    new_window->_height = height;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, 1);
    glfwWindowHint(GLFW_FOCUSED, 1);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#else
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, false);
#endif

    if(share != nullptr)
    {
        new_window->_glfw_window = glfwCreateWindow(width, height, title.c_str(), monitor, share->_glfw_window);
    }
    else
    {
        new_window->_glfw_window = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
    }

    if(new_window->_glfw_window == nullptr)
    {
        std::runtime_error("GLFW window creation failed");
    }

    make_context_current(new_window);

    glfwSetKeyCallback(new_window->_glfw_window, &EventHandler::on_window_key_press);
    glfwSetCharCallback(new_window->_glfw_window, &EventHandler::on_window_char);
    glfwSetMouseButtonCallback(new_window->_glfw_window, &EventHandler::on_window_button_press);
    glfwSetCursorPosCallback(new_window->_glfw_window, &EventHandler::on_window_move_cursor);
    glfwSetScrollCallback(new_window->_glfw_window, &EventHandler::on_window_scroll);
    glfwSetCursorEnterCallback(new_window->_glfw_window, &EventHandler::on_window_enter);

    glfwSetWindowUserPointer(new_window->_glfw_window, new_window);

    _windows.push_back(new_window);

    make_context_current(previous_context);

    return new_window;
}

bool should_close()
{
    if(_windows.empty())
        return true;

    std::list<Window*> to_delete;
    for(const auto& window : _windows)
    {
        if(glfwWindowShouldClose(window->_glfw_window))
        {
            to_delete.push_back(window);
        }
    }

    if(!to_delete.empty())
    {
        for(auto& window : to_delete)
        {
            ImGui_ImplGlfwGL3_Shutdown();

            glfwDestroyWindow(window->_glfw_window);

            delete window;

            _windows.remove(window);
        }
    }

    return _windows.empty();
}

void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* param)
{
    switch(severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
    {
        fprintf(stderr, "GL_DEBUG_SEVERITY_HIGH: %s type = 0x%x, message = %s\n", (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, message);
    }
    break;
    case GL_DEBUG_SEVERITY_MEDIUM:
    {
        fprintf(stderr, "GL_DEBUG_SEVERITY_MEDIUM: %s type = 0x%x, message = %s\n", (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, message);
    }
    break;
    case GL_DEBUG_SEVERITY_LOW:
    {
        fprintf(stderr, "GL_DEBUG_SEVERITY_LOW: %s type = 0x%x, message = %s\n", (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, message);
    }
    break;
    default:
        // ignore
        break;
    }
}

struct benchmark_entry
{
    uint64_t frame;
    float frame_time;
    float physical_texture_use;
    uint64_t update_throughput;
    uint64_t ooc_request_count;
    uint64_t ooc_loaded_count;
};

std::vector<benchmark_entry> benchmark_entries;
scm::shared_ptr<scm::core> _scm_core;

int main(int argc, char* argv[])
{
    _scm_core.reset(new scm::core(0, nullptr));

    vt::VTConfig::CONFIG_PATH = "config_demo_do_not_modify.ini";
    vt::VTConfig::get_instance().define_size_physical_texture(64, 8192);

    uint32_t data_world_map_id = vt::CutDatabase::get_instance().register_dataset("earth_colour_86400x43200_256x256_1_rgb.atlas");
    uint32_t data_world_elevation_map_id = vt::CutDatabase::get_instance().register_dataset("earth_elevation_43200x21600_256x256_1_rgb.atlas");
    uint32_t data_moon_map_id = vt::CutDatabase::get_instance().register_dataset("moon_colour_109164x54582_256x256_1_rgb.atlas");

    uint16_t view_id = vt::CutDatabase::get_instance().register_view();
    uint16_t primary_context_id = vt::CutDatabase::get_instance().register_context();

    uint64_t cut_map_id = vt::CutDatabase::get_instance().register_cut(data_world_map_id, view_id, primary_context_id);
    uint64_t cut_map_elevation_id = vt::CutDatabase::get_instance().register_cut(data_world_elevation_map_id, view_id, primary_context_id);
    uint64_t cut_moon_id = vt::CutDatabase::get_instance().register_cut(data_moon_map_id, view_id, primary_context_id);

    // Registration of resources has to happen before cut update start
    _cut_update = &vt::CutUpdate::get_instance();
    _cut_update->start();

    glfwSetErrorCallback(EventHandler::on_error);

    if(!glfwInit())
    {
        std::runtime_error("GLFW initialisation failed");
    }

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    glfwWindowHint(GLFW_DECORATED, GL_FALSE);

    Window* primary_window = create_window(mode->width, mode->height, "VT Demo", monitor, nullptr);

    make_context_current(primary_window);

#ifndef NDEBUG
    // VSYNC
    glfwSwapInterval(1);
#endif

    auto* vtrenderer = new vt::VTRenderer();

    vtrenderer->add_view(view_id, primary_window->_width, primary_window->_height, primary_window->_scale);
    vtrenderer->add_context(primary_context_id);

    vtrenderer->add_data(cut_map_id, primary_context_id, data_world_map_id);
    vtrenderer->add_data(cut_map_elevation_id, primary_context_id, data_world_elevation_map_id);
    vtrenderer->add_data(cut_moon_id, primary_context_id, data_moon_map_id);

#ifndef NDEBUG
    glewExperimental = GL_TRUE;
    glewInit();

    ImGui_ImplGlfwGL3_Init(primary_window->_glfw_window, false);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback((GLDEBUGPROC)debug_callback, nullptr);
#endif

#ifndef NDEBUG
    uint64_t frame_counter = 0;
#endif

    while(!should_close())
    {
#ifndef NDEBUG
        std::chrono::time_point<std::chrono::high_resolution_clock> frame_start = std::chrono::system_clock::now();
#endif
        glfwPollEvents();

        if(primary_window->_interaction == Window::Interaction::EARTH || primary_window->_interaction == Window::Interaction::MOON)
        {
            vtrenderer->update_view(view_id, primary_window->_width, primary_window->_height, primary_window->_scale, primary_window->_trackball_manipulator.transform_matrix());
        }
        else
        {
            scm::math::vec3f pos_camera = scm::math::vec3f(0.f, -5.f, 0.f);
            scm::math::vec3f pos_origin = scm::math::vec3f(0.f, 0.f, 0.f);
            scm::math::vec3f up = scm::math::vec3f(1.f, 0.f, 0.f);
            scm::math::mat4f view_mat = scm::math::make_look_at_matrix(pos_camera, pos_origin, up);
            vtrenderer->update_view(view_id, primary_window->_width, primary_window->_height, 1.0f, view_mat);
        }

        vtrenderer->clear_buffers(primary_context_id);

        bool feedback_enabled;

        if(primary_window->_dataset == Window::Dataset::COLOR)
        {
            vtrenderer->render_earth(data_world_map_id,
                                     data_world_elevation_map_id,
                                     view_id,
                                     primary_context_id,
                                     primary_window->_interaction == Window::Interaction::EARTH,
                                     primary_window->_enable_hierarchical,
                                     primary_window->_vis,
                                     feedback_enabled);
        }
        else
        {
            vtrenderer->render_earth(data_world_elevation_map_id,
                                     data_world_elevation_map_id,
                                     view_id,
                                     primary_context_id,
                                     primary_window->_interaction == Window::Interaction::EARTH,
                                     primary_window->_enable_hierarchical,
                                     primary_window->_vis,
                                     feedback_enabled);
        }
        vtrenderer->render_moon(
            data_moon_map_id, view_id, primary_context_id, primary_window->_interaction == Window::Interaction::MOON, primary_window->_enable_hierarchical, primary_window->_vis, feedback_enabled);

#ifndef NDEBUG
        vtrenderer->extract_debug_cut(cut_map_id);
        vtrenderer->extract_debug_cut(cut_map_elevation_id);
        vtrenderer->extract_debug_cut_context(cut_map_id);

        ImGui_ImplGlfwGL3_NewFrame();

        vtrenderer->render_debug_cut(cut_map_id);
        vtrenderer->render_debug_cut(cut_map_elevation_id);
        vtrenderer->render_debug_context(cut_map_id);

        ImGui::Render();
#endif

        glfwSwapBuffers(primary_window->_glfw_window);

#ifdef COLLECT_BENCHMARKS

        std::chrono::duration<float> frame_seconds = std::chrono::high_resolution_clock::now() - frame_start;

        vt::CutDatabase* cut_db = &vt::CutDatabase::get_instance();

        float phys_mem_usage = (float)(cut_db->get_size_mem_interleaved() - cut_db->get_available_memory()) / cut_db->get_size_mem_interleaved();
        uint64_t update_throughput = 0;

        for(vt::cut_map_entry_type cut_entry : (*cut_db->get_cut_map()))
        {
            vt::Cut* cut = cut_db->start_reading_cut(cut_entry.first);
            if(!cut->is_drawn())
            {
                cut_db->stop_reading_cut(cut_entry.first);
                continue;
            }

            update_throughput += vt::VTConfig::get_instance().get_size_tile() * vt::VTConfig::get_instance().get_size_tile() * 4 * cut->get_front()->get_mem_slots_updated().size();

            cut_db->stop_reading_cut(cut_entry.first);
        }

        uint64_t ooc_requested = vt::CutDatabase::get_instance().get_tile_provider()->get_requested();
        uint64_t ooc_loaded = vt::CutDatabase::get_instance().get_tile_provider()->get_loaded();

        benchmark_entries.push_back({frame_counter++, frame_seconds.count(), phys_mem_usage, update_throughput, ooc_requested, ooc_loaded});
#endif
    }

    _cut_update->stop();

    delete vtrenderer;

#ifdef COLLECT_BENCHMARKS
    uint32_t size_cache_ram = vt::VTConfig::get_instance().get_size_ram_cache();
    uint32_t size_cache_vram =
        vt::VTConfig::get_instance().get_phys_tex_layers() * vt::VTConfig::get_instance().get_phys_tex_px_width() * vt::VTConfig::get_instance().get_phys_tex_px_width() * 4 / 1024 / 1024;

    std::ofstream fs("benchmark_" + std::to_string(size_cache_ram) + "_" + std::to_string(size_cache_vram) + ".csv");
    text::csv::csv_ostream csvs(fs);

    csvs << "frame"
         << "frame_time"
         << "physical texture use"
         << "update throughput"
         << "ooc requested"
         << "ooc loaded" << text::csv::endl;

    for(auto entry : benchmark_entries)
    {
        csvs << std::to_string(entry.frame).c_str() << entry.frame_time << entry.physical_texture_use << std::to_string(entry.update_throughput).c_str()
             << std::to_string(entry.ooc_request_count).c_str() << std::to_string(entry.ooc_loaded_count).c_str() << text::csv::endl;
    }
#endif

    return EXIT_SUCCESS;
}