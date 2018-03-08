#ifndef LAMURE_VTCONFIG_H
#define LAMURE_VTCONFIG_H

#include <lamure/vt/common.h>

#include <fstream>
#include <iostream>

#include <lamure/vt/ext/SimpleIni.h>

#include <scm/core.h>
#include <scm/core/io/tools.h>
#include <scm/core/pointer_types.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>
#include <scm/log.h>

#include <scm/gl_core.h>

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/manipulators/trackball_manipulator.h>
#include <scm/gl_util/primitives/box.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/primitives/wavefront_obj.h>

namespace vt
{
class VTConfig
{
  public:
    enum FORMAT_TEXTURE
    {
        RGBA8,
        RGB8,
        R8
    };

    static std::string CONFIG_PATH;

    static VTConfig &get_instance()
    {
        static VTConfig instance(CONFIG_PATH.c_str());
        return instance;
    }
    VTConfig(VTConfig const &) = delete;
    void operator=(VTConfig const &) = delete;

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

    uint16_t get_byte_stride() const
    {
        uint8_t _byte_stride = 0;
        switch(_format_texture)
        {
        case VTConfig::FORMAT_TEXTURE::RGBA8:
            _byte_stride = 4;
            break;
        case VTConfig::FORMAT_TEXTURE::RGB8:
            _byte_stride = 3;
            break;
        case VTConfig::FORMAT_TEXTURE::R8:
            _byte_stride = 1;
            break;
        }

        return _byte_stride;
    }

    uint16_t get_size_tile() const;
    uint16_t get_size_padding() const;
    const std::string &get_name_texture() const;
    const std::string &get_name_mipmap() const;
    FORMAT_TEXTURE get_format_texture() const;
    bool is_verbose() const;
    uint16_t get_depth_quadtree() const;
    uint32_t get_size_index_texture() const;
    uint32_t get_size_physical_texture() const;
    uint32_t get_size_physical_update_throughput() const;
    uint32_t get_phys_tex_px_width() const;
    uint32_t get_phys_tex_tile_width() const;
    uint16_t get_phys_tex_layers() const;

private:
    VTConfig(const char *path_config)
    {
        read_config(path_config);
        _depth_quadtree = identify_depth();
        _size_index_texture = identify_size_index_texture();
        calculate_size_physical_texture();
    }

    // Sections
    static constexpr const char *TEXTURE_MANAGEMENT = "TEXTURE_MANAGEMENT";
    static constexpr const char *DEBUG = "DEBUG";

    // Texture management fields
    static constexpr const char *TILE_SIZE = "TILE_SIZE";
    static constexpr const char *TILE_PADDING = "TILE_PADDING";
    static constexpr const char *PHYSICAL_SIZE_MB = "PHYSICAL_SIZE_MB";
    static constexpr const char *PHYSICAL_UPDATE_THROUGHPUT_MB = "PHYSICAL_UPDATE_THROUGHPUT_MB";
    static constexpr const char *NAME_TEXTURE = "NAME_TEXTURE";
    static constexpr const char *NAME_MIPMAP = "NAME_MIPMAP";

    static constexpr const char *TEXTURE_FORMAT = "TEXTURE_FORMAT";
    static constexpr const char *TEXTURE_FORMAT_RGBA8 = "RGBA8";
    static constexpr const char *TEXTURE_FORMAT_RGB8 = "RGB8";
    static constexpr const char *TEXTURE_FORMAT_R8 = "R8";

    // Debug fields
    static constexpr const char *VERBOSE = "VERBOSE";

    static constexpr const char *UNDEF = "UNDEF";

  private:
    void read_config(const char *path_config);
    uint16_t identify_depth();
    uint32_t identify_size_index_texture();
    scm::math::vec2ui calculate_size_physical_texture();

    uint16_t _size_tile;
    uint16_t _size_padding;
    std::string _name_texture;
    std::string _name_mipmap;
    VTConfig::FORMAT_TEXTURE _format_texture;
    bool _verbose;

    uint16_t _depth_quadtree;
    uint32_t _size_index_texture;
    uint32_t _size_physical_texture;
    uint32_t _size_physical_update_throughput;

    uint32_t _phys_tex_px_width;
    uint32_t _phys_tex_tile_width;
    uint16_t _phys_tex_layers;
};
}

#endif // LAMURE_VTCONFIG_H
