#include <lamure/vt/Context.h>
namespace vt
{
Context::Context() { _config = new CSimpleIniA(true, false, false); }

uint16_t Context::get_size_tile() const { return _size_tile; }
const std::string &Context::get_name_texture() const { return _name_texture; }
const std::string &Context::get_name_mipmap() const { return _name_mipmap; }
bool Context::is_opt_run_in_parallel() const { return _opt_run_in_parallel; }
bool Context::is_opt_row_in_core() const { return _opt_row_in_core; }
Context::Config::FORMAT_TEXTURE Context::get_format_texture() const { return _format_texture; }
bool Context::is_keep_intermediate_data() const { return _keep_intermediate_data; }
bool Context::is_verbose() const { return _verbose; }

uint16_t Context::get_byte_stride() const
{
    uint8_t _byte_stride = 0;
    switch(_format_texture)
    {
    case Config::FORMAT_TEXTURE::RGBA8:
        _byte_stride = 4;
        break;
    case Config::FORMAT_TEXTURE::RGB8:
        _byte_stride = 3;
        break;
    case Config::FORMAT_TEXTURE::R8:
        _byte_stride = 1;
        break;
    }

    return _byte_stride;
}

void Context::start()
{
    if(!glfwInit())
    {
        std::runtime_error("GLFW initialisation failed");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);

    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    _window = glfwCreateWindow(mode->width, mode->height, "Virtual Texturing", glfwGetPrimaryMonitor(), nullptr);

    if(!_window)
    {
        glfwTerminate();
        std::runtime_error("GLFW window creation failed");
    }

    glfwMakeContextCurrent(_window);
    glfwSwapInterval(1);

    while(!glfwWindowShouldClose(_window))
    {
        _renderer->render(_window);

        glfwSwapBuffers(_window);
        glfwPollEvents();
    }

    glfwDestroyWindow(_window);
    glfwTerminate();
}
}