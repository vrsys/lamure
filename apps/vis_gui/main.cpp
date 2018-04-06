
#include "imgui_impl_glfw_gl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <list>

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
        //_trackball_manipulator.dolly(2.5f);
    }

    unsigned int _width;
    unsigned int _height;

    GLFWwindow *_glfw_window;

    //scm::gl::trackball_manipulator _trackball_manipulator;

    float _ref_rot_x;
    float _ref_rot_y;

    float _scale = 1.f;

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

class EventHandler
{
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
            break;
        }

        ImGui_ImplGlfwGL3_KeyCallback(glfw_window, key, scancode, action, mods);

    }
    static void on_window_char(GLFWwindow *glfw_window, unsigned int codepoint)
    {
        Window *window = (Window *)glfwGetWindowUserPointer(glfw_window);
        ImGui_ImplGlfwGL3_CharCallback(glfw_window, codepoint);

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

        ImGui_ImplGlfwGL3_MouseButtonCallback(glfw_window, button, action, mods);

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
            //window->_trackball_manipulator.rotation(window->_ref_rot_x, 0, x, 0);
        }
        break;
        case Window::MouseButtonState::RIGHT:
        {
            //window->_trackball_manipulator.rotation(0, window->_ref_rot_y, 0, y);
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
        window->_scale = std::max(window->_scale + (float)yoffset * 0.01f, -0.07f);

        ImGui_ImplGlfwGL3_ScrollCallback(glfw_window, xoffset, yoffset);

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

    //glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, false);


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

    glewExperimental = GL_TRUE;

    glewInit();

    ImGui_ImplGlfwGL3_Init(primary_window->_glfw_window, false);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback((GLDEBUGPROC)debug_callback, nullptr);

    while(!should_close())
    {
        glfwPollEvents();

        for(const auto &window : _windows)
        {
            make_context_current(window);

            if(window == primary_window)
            {

                glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
               

                ImGui_ImplGlfwGL3_NewFrame();

                ImGui::Begin("Hello_gui");

                ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

                if(ImGui::CollapsingHeader("Test"))
                {
                  ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0), "hello");
                  ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0), "gui");
                }

                ImGui::End();

                ImGui::Render();
            }
            else
            {
                // TODO
            }

            glfwSwapBuffers(window->_glfw_window);
        }
    }

    return EXIT_SUCCESS;
}
