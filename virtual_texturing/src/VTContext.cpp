#include <lamure/vt/VTContext.h>
#include <lamure/vt/ren/VTRenderer.h>
namespace vt
{
VTContext::VTContext() { _config = new CSimpleIniA(true, false, false); }

uint16_t VTContext::get_size_tile() const { return _size_tile; }
const std::string &VTContext::get_name_texture() const { return _name_texture; }
const std::string &VTContext::get_name_mipmap() const { return _name_mipmap; }
bool VTContext::is_opt_run_in_parallel() const { return _opt_run_in_parallel; }
bool VTContext::is_opt_row_in_core() const { return _opt_row_in_core; }
VTContext::Config::FORMAT_TEXTURE VTContext::get_format_texture() const { return _format_texture; }
bool VTContext::is_keep_intermediate_data() const { return _keep_intermediate_data; }
bool VTContext::is_verbose() const { return _verbose; }

uint16_t VTContext::get_byte_stride() const
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

// fast, safe touch: http://chris-sharpe.blogspot.de/2013/05/better-than-systemtouch.html
bool VTContext::touch(const std::string &pathname)
{
    int fd = open(pathname.c_str(), O_WRONLY | O_CREAT | O_NOCTTY | O_NONBLOCK, 0666);
    if(fd < 0) // Couldn't open that path.
    {
        std::cerr << __PRETTY_FUNCTION__ << ": Couldn't open() path \"" << pathname << "\"\n";
        return false;
    }
    int rc = utimensat(AT_FDCWD, pathname.c_str(), nullptr, 0);
    if(rc)
    {
        std::cerr << __PRETTY_FUNCTION__ << ": Couldn't utimensat() path \"" << pathname << "\"\n";
        return false;
    }
    return true;
}

void VTContext::start()
{
    if(!touch(_name_mipmap))
    {
        std::runtime_error("Mipmap file not found: " + _name_mipmap);
    }

    _depth_quadtree = identify_depth();
    _size_index_texture = identify_size_index_texture();
    _size_physical_texture = calculate_size_physical_texture();

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

    _vtrenderer = new VTRenderer(this);

    _trackball_manip.dolly(2.5f);

    while(!glfwWindowShouldClose(_window))
    {
        _vtrenderer->render(_window);

        glfwSwapBuffers(_window);
        glfwPollEvents();
    }

    glfwDestroyWindow(_window);
    glfwTerminate();
}
const scm::gl::trackball_manipulator &VTContext::get_trackball_manip() const { return _trackball_manip; }
uint16_t VTContext::get_depth_quadtree() const { return _depth_quadtree; }
uint16_t VTContext::identify_depth()
{
    std::streampos fsize = 0;
    std::ifstream file;

    file.open(_name_mipmap, std::ios::binary);

    if(!file.is_open())
    {
        throw std::runtime_error("Mipmap could not be read");
    }

    fsize = file.tellg();
    file.seekg(0, std::ios::end);
    fsize = file.tellg() - fsize;

    file.close();

    auto dim_x = (size_t)std::sqrt((size_t)fsize / get_byte_stride());

    return QuadTree::calculate_depth(dim_x, _size_tile);
}

uint32_t VTContext::identify_size_index_texture()
{
    std::streampos fsize = 0;
    std::ifstream file;

    file.open(_name_mipmap, std::ios::binary);

    if(!file.is_open())
    {
        throw std::runtime_error("Mipmap could not be read");
    }

    fsize = file.tellg();
    file.seekg(0, std::ios::end);
    fsize = file.tellg() - fsize;

    file.close();

    auto dim_x = (uint32_t)std::sqrt((size_t)fsize / get_byte_stride());

    return dim_x;
}

uint32_t VTContext::calculate_size_physical_texture()
{
    // TODO: define physical texture size, as huge as possible
    return 0;
}

bool VTContext::isToggle_phyiscal_texture_image_viewer() const { return toggle_phyiscal_texture_image_viewer; }
uint32_t VTContext::get_size_index_texture() const { return _size_index_texture; }
uint32_t VTContext::get_size_physical_texture() const { return _size_physical_texture; }
}