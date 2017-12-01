#ifndef LAMURE_CONTEXT_H
#define LAMURE_CONTEXT_H

#include "common.h"
#include <lamure/vt/ext/SimpleIni.h>
#include <lamure/vt/ren/Renderer.h>
namespace vt
{
class Context
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
        Context build()
        {
            Context context;

            if(this->_path_config != nullptr)
            {
                read_config(context, this->_path_config);
            }

            return context;
        }

      private:
        const char *_path_config;

        void read_config(Context &_context, const char *path_config)
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

    explicit Context();
    ~Context() = default;

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

  private:
    CSimpleIniA *_config;

    GLFWwindow *_window;

    Renderer *_renderer;

    uint16_t _size_tile;
    std::string _name_texture;
    std::string _name_mipmap;
    bool _opt_run_in_parallel;
    bool _opt_row_in_core;
    Config::FORMAT_TEXTURE _format_texture;
    bool _keep_intermediate_data;
    bool _verbose;
};
}

#endif // LAMURE_CONTEXT_H