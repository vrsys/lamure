#ifndef VT_CONTEXT_H
#define VT_CONTEXT_H

#include <lamure/vt/common.h>

namespace vt
{
class VTRenderer;
class CutUpdate;

template <typename priority_type>
class TileAtlas;
class VTContext
{
  public:
    class Config
    {
      public:
        // Sections
        static constexpr const char *TEXTURE_MANAGEMENT = "TEXTURE_MANAGEMENT";
        static constexpr const char *DEBUG = "DEBUG";

        // Texture management fields
        static constexpr const char *TILE_SIZE = "TILE_SIZE";
        static constexpr const char *NAME_TEXTURE = "NAME_TEXTURE";
        static constexpr const char *NAME_MIPMAP = "NAME_MIPMAP";

        static constexpr const char *OPT_RUN_IN_PARALLEL = "OPT_RUN_IN_PARALLEL";
        static constexpr const char *OPT_TILE_ROW_IN_CORE = "OPT_TILE_ROW_IN_CORE";

        static constexpr const char *TEXTURE_FORMAT = "TEXTURE_FORMAT";
        static constexpr const char *TEXTURE_FORMAT_RGBA8 = "RGBA8";
        static constexpr const char *TEXTURE_FORMAT_RGB8 = "RGB8";
        static constexpr const char *TEXTURE_FORMAT_R8 = "R8";

        // Debug fields
        static constexpr const char *KEEP_INTERMEDIATE_DATA = "KEEP_INTERMEDIATE_DATA";
        static constexpr const char *VERBOSE = "VERBOSE";

        static constexpr const char *UNDEF = "UNDEF";

        enum FORMAT_TEXTURE
        {
            RGBA8,
            RGB8,
            R8
        };

        static const FORMAT_TEXTURE which_texture_format(const char *_texture_format)
        {
            if(strcmp(_texture_format, TEXTURE_FORMAT_RGBA8) == 0)
            {
                return RGBA8;
            }
            else if(strcmp(_texture_format, TEXTURE_FORMAT_RGB8) == 0)
            {
                return RGB8;
            }
            else if(strcmp(_texture_format, TEXTURE_FORMAT_R8) == 0)
            {
                return R8;
            }
            throw std::runtime_error("Unknown texture format");
        }
    };

    class EventHandler
    {
      public:
        EventHandler();
        ~EventHandler() = default;

        enum MouseButtonState
        {
            LEFT = 0,
            WHEEL = 1,
            RIGHT = 2,
            IDLE = 3
        };

        static void on_error(int _err_code, const char *_err_msg);
        static void on_window_resize(GLFWwindow *_window, int _width, int _height);
        static void on_window_key_press(GLFWwindow *_window, int _key, int _scancode, int _action, int _mods);
        static void on_window_char(GLFWwindow *_window, unsigned int _codepoint);
        static void on_window_button_press(GLFWwindow *_window, int _button, int _action, int _mods);
        static void on_window_move_cursor(GLFWwindow *_window, double _xpos, double _ypos);
        static void on_window_scroll(GLFWwindow *_window, double _xoffset, double _yoffset);
        static void on_window_enter(GLFWwindow *_window, int _entered);

        const scm::gl::trackball_manipulator &get_trackball_manip() const;
        void set_trackball_manip(const scm::gl::trackball_manipulator &_trackball_manip);
        float get_initx() const;
        void set_initx(float _initx);
        float get_inity() const;
        void set_inity(float _inity);
        MouseButtonState get_mouse_button_state() const;
        void set_mouse_button_state(MouseButtonState _mouse_button_state);
        float get_dolly_sensitivity() const;
        void set_dolly_sensitivity(float _dolly_sensitivity);

        bool isToggle_phyiscal_texture_image_viewer() const;

      private:
        bool toggle_phyiscal_texture_image_viewer = true;

        // trackball -> mouse and x+y coord.
        scm::gl::trackball_manipulator _trackball_manip;
        float _initx;
        float _inity;

        float _ref_width;
        float _ref_height;

        // mouse button state
        MouseButtonState _mouse_button_state;

        // intensity of zoom
        float _dolly_sensitivity;
    };

    class Builder
    {
      public:
        Builder() = default;
        ~Builder() = default;
        Builder *with_path_config(const char *path_config)
        {
            this->_path_config = path_config;
            return this;
        }
        Builder *with_event_handler(EventHandler *_event_handler)
        {
            this->_event_handler = _event_handler;
            return this;
        }
        VTContext build()
        {
            VTContext context;

            if(this->_path_config != nullptr)
            {
                read_config(context, this->_path_config);
            }

            if(this->_event_handler != nullptr)
            {
                context.set_event_handler(_event_handler);
            }

            return context;
        }

      private:
        const char *_path_config;
        EventHandler *_event_handler;

        void read_config(VTContext &_context, const char *path_config)
        {
            if(_context._config->LoadFile(path_config) < 0)
            {
                throw std::runtime_error("Configuration file parsing error");
            }

            _context._size_tile = (uint16_t)atoi(_context._config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TILE_SIZE, Config::UNDEF));
            _context._name_texture = std::string(_context._config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF));
            _context._name_mipmap = std::string(_context._config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_MIPMAP, Config::UNDEF));
            _context._opt_run_in_parallel = atoi(_context._config->GetValue(Config::TEXTURE_MANAGEMENT, Config::OPT_RUN_IN_PARALLEL, Config::UNDEF)) == 1;
            _context._opt_row_in_core = atoi(_context._config->GetValue(Config::TEXTURE_MANAGEMENT, Config::OPT_TILE_ROW_IN_CORE, Config::UNDEF)) == 1;
            _context._format_texture = Config::which_texture_format(_context._config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TEXTURE_FORMAT, Config::UNDEF));
            _context._keep_intermediate_data = atoi(_context._config->GetValue(Config::DEBUG, Config::KEEP_INTERMEDIATE_DATA, Config::UNDEF)) == 1;
            _context._verbose = atoi(_context._config->GetValue(Config::DEBUG, Config::VERBOSE, Config::UNDEF)) == 1;
        }
    };

    ~VTContext();

    void start();

    uint16_t get_byte_stride() const;

    uint16_t get_size_tile() const;
    const std::string &get_name_texture() const;
    const std::string &get_name_mipmap() const;
    bool is_opt_run_in_parallel() const;
    bool is_opt_row_in_core() const;
    Config::FORMAT_TEXTURE get_format_texture() const;
    bool is_keep_intermediate_data() const;
    bool is_verbose() const;

    uint16_t get_depth_quadtree() const;
    uint32_t get_size_index_texture() const;
    uint32_t get_size_physical_texture() const;

    CutUpdate *get_cut_update() const;
    TileAtlas<priority_type> *get_atlas() const;
    VTRenderer *get_vtrenderer() const;
    EventHandler *get_event_handler() const;
    scm::math::vec2ui calculate_size_physical_texture(int physical_texture_max_size_mb);

    void set_event_handler(EventHandler *_event_handler);

  private:
    explicit VTContext();
    uint16_t identify_depth();
    uint32_t identify_size_index_texture();

    CSimpleIniA *_config;

    GLFWwindow *_window;
    EventHandler *_event_handler;

    VTRenderer *_vtrenderer;

    CutUpdate *_cut_update;
    TileAtlas<priority_type> *_atlas;

    uint16_t _size_tile;
    std::string _name_texture;
    std::string _name_mipmap;
    bool _opt_run_in_parallel;
    bool _opt_row_in_core;
    Config::FORMAT_TEXTURE _format_texture;
    bool _keep_intermediate_data;
    bool _verbose;

    uint16_t _depth_quadtree;
    uint32_t _size_index_texture;
    uint32_t _size_physical_texture;
};
}

#endif // VT_CONTEXT_H