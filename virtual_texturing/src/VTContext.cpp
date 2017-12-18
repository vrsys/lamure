#include <lamure/vt/VTContext.h>
#include <lamure/vt/ooc/TileAtlas.h>
#include <lamure/vt/ren/CutUpdate.h>
#include <lamure/vt/ren/VTRenderer.h>
namespace vt
{
VTContext::VTContext() : _debug() { _config = new CSimpleIniA(true, false, false); }

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

void VTContext::start()
{
    auto fileName = _name_mipmap + ".data";

    if(access((fileName).c_str(), F_OK) != -1)
    {
        std::runtime_error("Mipmap file not found: " + _name_mipmap);
    }

    _atlas = new vt::TileAtlas<priority_type>(fileName, _size_tile * _size_tile * get_byte_stride());

    _depth_quadtree = identify_depth();
    _size_index_texture = identify_size_index_texture();

    glfwSetErrorCallback(&VTContext::EventHandler::on_error);

    if(!glfwInit())
    {
        std::runtime_error("GLFW initialisation failed");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_FALSE);

    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    _window = glfwCreateWindow(mode->width, mode->height, "Virtual Texturing", glfwGetPrimaryMonitor(), nullptr);

    if(!_window)
    {
        glfwTerminate();
        std::runtime_error("GLFW window creation failed");
    }

    glfwMakeContextCurrent(_window);

    glfwSetWindowUserPointer(_window, this);
    glfwSetWindowSizeCallback(_window, &VTContext::EventHandler::on_window_resize);

    glfwSwapInterval(1);

    glfwSetKeyCallback(_window, &VTContext::EventHandler::on_window_key_press);
    glfwSetCharCallback(_window, &VTContext::EventHandler::on_window_char);
    glfwSetMouseButtonCallback(_window, &VTContext::EventHandler::on_window_button_press);
    glfwSetCursorPosCallback(_window, &VTContext::EventHandler::on_window_move_cursor);
    glfwSetScrollCallback(_window, &VTContext::EventHandler::on_window_scroll);
    glfwSetCursorEnterCallback(_window, &VTContext::EventHandler::on_window_enter);

    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    _cut_update = new CutUpdate(this);
    _cut_update->start();

    _vtrenderer = new VTRenderer(this, (uint32_t)mode->width, (uint32_t)mode->height);

    glewExperimental = GL_TRUE;

    glewInit();

    if(_show_debug_view)
    {
        ImGui_ImplGlfwGL3_Init(_window, false);
    }

    while(!glfwWindowShouldClose(_window))
    {
        glfwPollEvents();

        _vtrenderer->render();
        if(_show_debug_view)
        {
            _vtrenderer->render_debug_view();
        }

        glfwSwapBuffers(_window);
    }

    if(_show_debug_view)
    {
        ImGui_ImplGlfwGL3_Shutdown();
    }

    std::cout << "rendering stopped" << std::endl;

    _cut_update->stop();

    std::cout << "cut update stopped" << std::endl;

    glfwDestroyWindow(_window);
    glfwTerminate();
}
uint16_t VTContext::get_depth_quadtree() const { return _depth_quadtree; }
uint16_t VTContext::identify_depth()
{
    size_t fsize = 0;
    std::ifstream file;

    file.open(_name_mipmap + ".data", std::ifstream::in | std::ifstream::binary);

    if(!file.is_open())
    {
        throw std::runtime_error("Mipmap could not be read");
    }

    fsize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::end);
    fsize = static_cast<size_t>(file.tellg()) - fsize;

    file.close();

    auto texel_count = static_cast<uint32_t>((size_t)fsize / get_byte_stride());

    size_t count_tiled = texel_count / _size_tile / _size_tile;

    size_t depth = QuadTree::get_depth_of_node(count_tiled - 1);

    return (uint16_t)depth;
}

uint32_t VTContext::identify_size_index_texture() { return (uint32_t)std::pow(2, _depth_quadtree); }

scm::math::vec2ui VTContext::calculate_size_physical_texture()
{
    // TODO: define physical texture size, as huge as possible
    /* uint32_t number_of_tiles =  _size_physical_texture * 1024 * 1024 / tilesize;
     uint32_t tiles_per_dim_x = 8192 /  _size_tile;
     uint64_t tiles_per_dim_y = ceil((double)number_of_tiles / (double)tiles_per_dim_x);
     std::cout << "phy_tex_dim: " << tiles_per_dim_x << " , " << tiles_per_dim_y << std::endl;*/
    uint32_t input_in_byte = _size_physical_texture * 1024 * 1024;
    uint32_t tilesize = (uint32_t)_size_tile * _size_tile * get_byte_stride();
    uint32_t total_amount_of_tiles = input_in_byte / tilesize;
    uint32_t tiles_per_dim_x = (uint32_t)floor(sqrt(total_amount_of_tiles));
    uint32_t tiles_per_dim_y = total_amount_of_tiles / tiles_per_dim_x;

    std::cout << tiles_per_dim_x << " " << tiles_per_dim_y << std::endl;
    return scm::math::vec2ui(tiles_per_dim_x, tiles_per_dim_y);
}

bool VTContext::EventHandler::isToggle_phyiscal_texture_image_viewer() const { return toggle_phyiscal_texture_image_viewer; }
uint32_t VTContext::get_size_index_texture() const { return _size_index_texture; }
VTRenderer *VTContext::get_vtrenderer() const { return _vtrenderer; }
VTContext::EventHandler *VTContext::get_event_handler() const { return _event_handler; }

void VTContext::set_event_handler(VTContext::EventHandler *_event_handler) { VTContext::_event_handler = _event_handler; }
void VTContext::set_debug_view(bool show_debug_view) { this->_show_debug_view = show_debug_view; }
VTContext::~VTContext() { delete _cut_update; }
TileAtlas<priority_type> *VTContext::get_atlas() const { return _atlas; }
CutUpdate *VTContext::get_cut_update() { return _cut_update; }
VTContext::Debug *VTContext::get_debug() { return &_debug; }

void VTContext::EventHandler::on_error(int _err_code, const char *_err_msg) { throw std::runtime_error(_err_msg); }
void VTContext::EventHandler::on_window_resize(GLFWwindow *_window, int _width, int _height)
{
    auto vtcontext(static_cast<VTContext *>(glfwGetWindowUserPointer(_window)));

    vtcontext->_event_handler->_ref_width = _width;
    vtcontext->_event_handler->_ref_height = _height;

    vtcontext->get_vtrenderer()->resize(_width, _height);
}
void VTContext::EventHandler::on_window_key_press(GLFWwindow *_window, int _key, int _scancode, int _action, int _mods)
{
    if(!_action == GLFW_PRESS)
    {
        return;
    }

    auto vtcontext(static_cast<VTContext *>(glfwGetWindowUserPointer(_window)));

    std::vector<uint8_t> cpu_idx_texture_buffer_state;
    switch(_key)
    {
    case GLFW_KEY_ESCAPE:
        std::cout << "should close" << std::endl;
        glfwSetWindowShouldClose(_window, GL_TRUE);
        break;
    case GLFW_KEY_SPACE:
        vtcontext->_event_handler->toggle_phyiscal_texture_image_viewer = !vtcontext->_event_handler->toggle_phyiscal_texture_image_viewer;
        break;
    }

    ImGui_ImplGlfwGL3_KeyCallback(_window, _key, _scancode, _action, _mods);
}
void VTContext::EventHandler::on_window_char(GLFWwindow *_window, unsigned int _codepoint) { ImGui_ImplGlfwGL3_CharCallback(_window, _codepoint); }
void VTContext::EventHandler::on_window_button_press(GLFWwindow *_window, int _button, int _action, int _mods)
{
    auto vtcontext(static_cast<VTContext *>(glfwGetWindowUserPointer(_window)));

    if(_button == GLFW_MOUSE_BUTTON_LEFT && _action == GLFW_PRESS)
    {
        vtcontext->get_event_handler()->_mouse_button_state = MouseButtonState::LEFT;
    }
    else if(_button == GLFW_MOUSE_BUTTON_MIDDLE && _action == GLFW_PRESS)
    {
        vtcontext->get_event_handler()->_mouse_button_state = MouseButtonState::WHEEL;
    }
    else if(_button == GLFW_MOUSE_BUTTON_RIGHT && _action == GLFW_PRESS)
    {
        vtcontext->get_event_handler()->_mouse_button_state = MouseButtonState::RIGHT;
    }
    else
    {
        vtcontext->get_event_handler()->_mouse_button_state = MouseButtonState::IDLE;
    }

    ImGui_ImplGlfwGL3_MouseButtonCallback(_window, _button, _action, _mods);
}
void VTContext::EventHandler::on_window_move_cursor(GLFWwindow *_window, double _xpos, double _ypos)
{
    auto vtcontext(static_cast<VTContext *>(glfwGetWindowUserPointer(_window)));

    EventHandler *event_handler = vtcontext->get_event_handler();

    switch(event_handler->_mouse_button_state)
    {
    case MouseButtonState::LEFT:
    {
        float x = ((float)_xpos / event_handler->_ref_width - 0.5f) * 10.0f;
        event_handler->_trackball_manip.rotation(event_handler->_ref_rot_x, 0, x, 0);
        event_handler->_ref_rot_x = x;
    }
    break;
    case MouseButtonState::RIGHT:
    {
        float y = ((float)_ypos / event_handler->_ref_height - 0.5f) * 10.0f;
        event_handler->_trackball_manip.rotation(0, event_handler->_ref_rot_y, 0, y);
        event_handler->_ref_rot_y = y;
    }
    break;
    }
}
void VTContext::EventHandler::on_window_scroll(GLFWwindow *_window, double _xoffset, double _yoffset)
{
    auto vtcontext(static_cast<VTContext *>(glfwGetWindowUserPointer(_window)));

    EventHandler *event_handler = vtcontext->get_event_handler();

    event_handler->_trackball_manip.dolly((float)_yoffset);

    ImGui_ImplGlfwGL3_ScrollCallback(_window, _xoffset, _yoffset);
}
void VTContext::EventHandler::on_window_enter(GLFWwindow *_window, int _entered)
{
    // TODO
}
const scm::gl::trackball_manipulator &VTContext::EventHandler::get_trackball_manip() const { return _trackball_manip; }
void VTContext::EventHandler::set_trackball_manip(const scm::gl::trackball_manipulator &_trackball_manip) { this->_trackball_manip = _trackball_manip; }
VTContext::EventHandler::MouseButtonState VTContext::EventHandler::get_mouse_button_state() const { return _mouse_button_state; }
void VTContext::EventHandler::set_mouse_button_state(VTContext::EventHandler::MouseButtonState _mouse_button_state) { this->_mouse_button_state = _mouse_button_state; }
VTContext::EventHandler::EventHandler()
{
    _mouse_button_state = MouseButtonState::IDLE;

    _trackball_manip.dolly(2.5f);
}
std::deque<float> &VTContext::Debug::get_fps() { return _fps; }
float VTContext::Debug::get_mem_slots_busy() { return _mem_slots_busy; }
void VTContext::Debug::set_mem_slots_busy(float _mem_slots_busy) { Debug::_mem_slots_busy = _mem_slots_busy; }
const std::string &VTContext::Debug::get_cut_string() const { return _string_cut; }
void VTContext::Debug::set_cut_string(const string &_cut_string) { Debug::_string_cut = _cut_string; }
const std::string &VTContext::Debug::get_mem_slots_string() const { return _string_mem_slots; }
void VTContext::Debug::set_mem_slots_string(const std::string &_mem_slots_string) { Debug::_string_mem_slots = _mem_slots_string; }
const std::string &VTContext::Debug::get_index_string() const { return _string_index; }
void VTContext::Debug::set_index_string(const std::string &_index_string) { Debug::_string_index = _index_string; }
std::deque<float> &VTContext::Debug::get_cut_swap_times() { return _times_cut_swap; }
std::deque<float> &VTContext::Debug::get_cut_dispatch_times() { return _times_cut_dispatch; }
std::deque<float> &VTContext::Debug::get_apply_times() { return _times_apply; }
size_t VTContext::Debug::get_size_mem_cut() const { return _size_mem_cut; }
void VTContext::Debug::set_size_mem_cut(size_t _size_mem_cut) { Debug::_size_mem_cut = _size_mem_cut; }
const std::string &VTContext::Debug::get_feedback_string() const { return _string_feedback; }
void VTContext::Debug::set_feedback_string(const std::string &feedback_string) { Debug::_string_feedback = feedback_string; }
}