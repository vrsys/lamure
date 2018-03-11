#include "VTRenderer.h"
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

class EventHandler
{
  public:
    enum MouseButtonState
    {
        LEFT = 0,
        WHEEL = 1,
        RIGHT = 2,
        IDLE = 3
    };

    static void on_error(int _err_code, const char *_err_msg);
    static void on_window_resize(GLFWwindow *_window, int _width, int _height);
};

void EventHandler::on_error(int _err_code, const char *_err_msg) { throw std::runtime_error(_err_msg); }
void EventHandler::on_window_resize(GLFWwindow *_window, int _width, int _height)
{
    // TODO
    //    vtcontext->_event_handler->_ref_width = _width;
    //    vtcontext->_event_handler->_ref_height = _height;

    //    vtcontext->get_vtrenderer()->resize(_width, _height);
}

struct Window
{
    unsigned int _width;
    unsigned int _height;

    GLFWwindow *_glfw_window;
};

std::list<Window *> _windows;
Window *_current_context = nullptr;

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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
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

    glfwSetWindowSizeCallback(new_window->_glfw_window, EventHandler::on_window_resize);
    glfwSetInputMode(new_window->_glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

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
            glfwDestroyWindow(window->_glfw_window);

            delete window;

            _windows.remove(window);
        }
    }

    return _windows.empty();
}

int main(int argc, char *argv[])
{
    if(argc == 1 || !cmd_option_exists(argv, argv + argc, "-c") || !cmd_option_exists(argv, argv + argc, "-f"))
    {
        std::cout << "Texturing context: " << argv[0] << " <flags> -c <config> -f <*.atlas.data>" << std::endl;
        return -1;
    }

    std::string file_config = std::string(get_cmd_option(argv, argv + argc, "-c"));
    std::string file_atlas = std::string(get_cmd_option(argv, argv + argc, "-f"));

    vt::VTConfig::CONFIG_PATH = file_config;
    vt::VTConfig::get_instance().define_size_physical_texture(16, 256000);

    uint32_t data_id = vt::CutDatabase::get_instance().register_dataset(file_atlas);
    uint16_t view_id = vt::CutDatabase::get_instance().register_view();
    uint16_t primary_context_id = vt::CutDatabase::get_instance().register_context();

    uint64_t cut_id = vt::CutDatabase::get_instance().register_cut(data_id, view_id, primary_context_id);

    // Registration of resources has to happen before cut update start
    auto *cut_update = new vt::CutUpdate();
    cut_update->start();

    glfwSetErrorCallback(EventHandler::on_error);

    if(!glfwInit())
    {
        std::runtime_error("GLFW initialisation failed");
    }

    Window *primary_window = create_window(600, 600, "First", nullptr, nullptr);
    create_window(600, 600, "Second", nullptr, primary_window);

    make_context_current(primary_window);

    glfwSwapInterval(1);

    auto *vtrenderer = new vt::VTRenderer(cut_update);

    vtrenderer->add_data(cut_id, data_id, std::string(LAMURE_PRIMITIVES_DIR) + "/world_smooth.obj");
    vtrenderer->add_view(view_id, primary_window->_width, primary_window->_height, 0.5f);
    vtrenderer->add_context(primary_context_id);

    scm::gl::trackball_manipulator trackball_manipulator;
    trackball_manipulator.dolly(2.5f);

    while(!should_close())
    {
        glfwPollEvents();

        for(const auto &window : _windows)
        {
            make_context_current(window);

            if(window == primary_window)
            {
                vtrenderer->update_view(view_id, window->_width, window->_height, 0.5f, trackball_manipulator.transform_matrix());
                vtrenderer->render(data_id, view_id, primary_context_id);
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