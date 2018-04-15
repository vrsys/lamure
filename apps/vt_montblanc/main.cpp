#include "VTRenderer.h"
#include "imgui_impl_glfw_gl3.h"
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

char *get_cmd_option(char **begin, char **end, const std::string &option)
{
    char **it = std::find(begin, end, option);
    if(it != end && ++it != end)
        return *it;
    return nullptr;
}

bool cmd_option_exists(char **begin, char **end, const std::string &option) { return std::find(begin, end, option) != end; }

struct Window
{
    Window()
    {
        _mouse_button_state = MouseButtonState::IDLE;
        _trackball_manipulator.dolly(2.5f);
    }

    unsigned int _width;
    unsigned int _height;

    GLFWwindow *_glfw_window;

    scm::gl::trackball_manipulator _trackball_manipulator;

    float _ref_rot_x;
    float _ref_rot_y;

    float _scale = 0.16f;

    enum MouseButtonState
    {
        LEFT = 0,
        WHEEL = 1,
        RIGHT = 2,
        IDLE = 3
    };

    MouseButtonState _mouse_button_state;
};

std::list<Window *> _windows;
Window *_current_context = nullptr;
vt::CutUpdate *_cut_update = nullptr;

class EventHandler
{
  private:
    static float calculate_factor(float scale)
    {
        return -1.0f/109.0f * scale * scale + 1081.0f/10900.0f * scale + 0.01f;
    }

  public:
    static void on_error(int _err_code, const char *err_msg) { throw std::runtime_error(err_msg); }
    static void on_window_resize(GLFWwindow *glfw_window, int width, int height)
    {
        Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
        window->_height = (uint32_t)height;
        window->_width = (uint32_t)width;
    }
    static void on_window_key_press(GLFWwindow *glfw_window, int key, int scancode, int action, int mods)
    {
        Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
        switch(key)
        {
            case GLFW_KEY_ESCAPE:
                std::cout << "should close" << std::endl;
                glfwSetWindowShouldClose(glfw_window, GL_TRUE);
                break;
            case GLFW_KEY_P:
                if (action == GLFW_PRESS) {
                    _cut_update->set_freeze_dispatch(!_cut_update->get_freeze_dispatch());
                }
                break;
            case GLFW_KEY_RIGHT:
                if (action != GLFW_RELEASE) {
                    window->_trackball_manipulator.translation(-calculate_factor(window->_scale), 0.0f);
                }
                break;
            case GLFW_KEY_LEFT:
                if (action != GLFW_RELEASE) {
                    window->_trackball_manipulator.translation(calculate_factor(window->_scale), 0.0f);
                }
                break;
            case GLFW_KEY_UP:
                if (action != GLFW_RELEASE) {
                    window->_trackball_manipulator.translation(0.0f, -calculate_factor(window->_scale));
                }
                break;
            case GLFW_KEY_DOWN:
                if (action != GLFW_RELEASE) {
                    window->_trackball_manipulator.translation(0.0f, calculate_factor(window->_scale));
                }
                break;
        }

#ifndef NDEBUG
        ImGui_ImplGlfwGL3_KeyCallback(glfw_window, key, scancode, action, mods);
#endif
    }
    static void on_window_char(GLFWwindow *glfw_window, unsigned int codepoint)
    {
        Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
#ifndef NDEBUG
        ImGui_ImplGlfwGL3_CharCallback(glfw_window, codepoint);
#endif
    }
    static void on_window_button_press(GLFWwindow *glfw_window, int button, int action, int mods)
    {
        Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
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
    static void on_window_move_cursor(GLFWwindow *glfw_window, double xpos, double ypos)
    {
        Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);

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
    static void on_window_scroll(GLFWwindow *glfw_window, double xoffset, double yoffset)
    {
        Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);

        float scale_factor = -1.0f/109.0f * window->_scale * window->_scale + 1081.0f/10900.0f * window->_scale + 0.01f;
        window->_scale = std::max(window->_scale + (float)yoffset * scale_factor, -0.1f);

#ifndef NDEBUG
        ImGui_ImplGlfwGL3_ScrollCallback(glfw_window, xoffset, yoffset);
#endif
    }
    static void on_window_enter(GLFWwindow *glfw_window, int entered)
    {
        Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
        // TODO
    }
};

void make_context_current(Window *_window)
{
    if(_window != nullptr)
    {
        glfwMakeContextCurrent(_window->_glfw_window);
        _current_context = _window;
    }
}

Window *create_window(unsigned int width, unsigned int height, const std::string &title, GLFWmonitor *monitor, Window *share)
{
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

    glfwSetKeyCallback         (new_window->_glfw_window, &EventHandler::on_window_key_press);
    glfwSetCharCallback        (new_window->_glfw_window, &EventHandler::on_window_char);
    glfwSetMouseButtonCallback (new_window->_glfw_window, &EventHandler::on_window_button_press);
    glfwSetCursorPosCallback   (new_window->_glfw_window, &EventHandler::on_window_move_cursor);
    glfwSetScrollCallback      (new_window->_glfw_window, &EventHandler::on_window_scroll);
    glfwSetCursorEnterCallback (new_window->_glfw_window, &EventHandler::on_window_enter);

    glfwSetWindowUserPointer(new_window->_glfw_window, new_window);

    _windows.push_back(new_window);

    make_context_current(previous_context);

    return new_window;
}

bool should_close()
{
    if(_windows.empty())
        return true;

    std::list<Window *> to_delete;
    for(const auto &window : _windows)
    {
        if(glfwWindowShouldClose(window->_glfw_window))
        {
            to_delete.push_back(window);
        }
    }

    if(!to_delete.empty())
    {
        for(auto &window : to_delete)
        {
            ImGui_ImplGlfwGL3_Shutdown();

            glfwDestroyWindow(window->_glfw_window);

            delete window;

            _windows.remove(window);
        }
    }

    return _windows.empty();
}

void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *param)
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

int main(int argc, char *argv[])
{
    vt::VTConfig::CONFIG_PATH = "configuration_template.ini";
    vt::VTConfig::get_instance().define_size_physical_texture(64, 8192);

    uint32_t data_world_map_id = vt::CutDatabase::get_instance().register_dataset(
            "/mnt/terabytes_of_textures/montblanc/montblanc_w1202116_h304384.atlas");

    uint16_t view_id = vt::CutDatabase::get_instance().register_view();
    uint16_t primary_context_id = vt::CutDatabase::get_instance().register_context();

    uint64_t cut_map_id = vt::CutDatabase::get_instance().register_cut(data_world_map_id, view_id, primary_context_id);

    // Registration of resources has to happen before cut update start
    _cut_update = &vt::CutUpdate::get_instance();
    _cut_update->start();

    glfwSetErrorCallback(EventHandler::on_error);

    if(!glfwInit())
    {
        std::runtime_error("GLFW initialisation failed");
    }

    Window *primary_window = create_window(1920, 1080, "First", nullptr, nullptr);
    // TODO
    // create_window(600, 600, "Second", nullptr, primary_window);

    make_context_current(primary_window);

    glfwSwapInterval(1);

    auto *vtrenderer = new vt::VTRenderer(_cut_update);

    vtrenderer->add_data(cut_map_id, data_world_map_id);
    vtrenderer->add_view(view_id, primary_window->_width, primary_window->_height, primary_window->_scale);
    vtrenderer->add_context(primary_context_id);

#ifndef NDEBUG
    glewExperimental = GL_TRUE;

    glewInit();

    ImGui_ImplGlfwGL3_Init(primary_window->_glfw_window, false);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback((GLDEBUGPROC)debug_callback, nullptr);
#endif

    while(!should_close())
    {
        glfwPollEvents();

        for(const auto &window : _windows)
        {
            make_context_current(window);

            if(window == primary_window)
            {
                vtrenderer->update_view(view_id,
                                        window->_width, window->_height,
                                        window->_scale,
                                        window->_trackball_manipulator.transform_matrix());

                vtrenderer->clear_buffers(primary_context_id);

                vtrenderer->render(data_world_map_id, view_id, primary_context_id);

                vtrenderer->collect_feedback(primary_context_id);
#ifndef NDEBUG
                vtrenderer->extract_debug_cut(data_world_map_id, view_id, primary_context_id);
                //vtrenderer->extract_debug_cut(data_world_elevation_id, view_id, primary_context_id);
                vtrenderer->extract_debug_context(primary_context_id);

                ImGui_ImplGlfwGL3_NewFrame();

                vtrenderer->render_debug_cut(data_world_map_id, view_id, primary_context_id);
                //vtrenderer->render_debug_cut(data_world_elevation_id, view_id, primary_context_id);
                vtrenderer->render_debug_context(primary_context_id);

                ImGui::Render();
#endif
            }
            else
            {
                // TODO
            }

            glfwSwapBuffers(window->_glfw_window);
        }
    }

    _cut_update->stop();

    return EXIT_SUCCESS;
}