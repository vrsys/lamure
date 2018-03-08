#include <lamure/vt/QuadTree.h>
#include <lamure/vt/VTConfig.h>

namespace vt
{
std::string vt::VTConfig::CONFIG_PATH = "";

void VTConfig::read_config(const char *path_config)
{
    CSimpleIniA *ini_config = new CSimpleIniA(true, false, false);

    if(ini_config->LoadFile(path_config) < 0)
    {
        throw std::runtime_error("Configuration file parsing error");
    }

    _size_tile = (uint16_t)atoi(ini_config->GetValue(VTConfig::TEXTURE_MANAGEMENT, VTConfig::TILE_SIZE, VTConfig::UNDEF));
    _size_padding = (uint16_t)atoi(ini_config->GetValue(VTConfig::TEXTURE_MANAGEMENT, VTConfig::TILE_PADDING, VTConfig::UNDEF));
    _size_physical_texture = (uint32_t)atoi(ini_config->GetValue(VTConfig::TEXTURE_MANAGEMENT, VTConfig::PHYSICAL_SIZE_MB, VTConfig::UNDEF));
    _size_physical_update_throughput = (uint32_t)atoi(ini_config->GetValue(VTConfig::TEXTURE_MANAGEMENT, VTConfig::PHYSICAL_SIZE_MB, VTConfig::UNDEF));
    _name_texture = std::string(ini_config->GetValue(VTConfig::TEXTURE_MANAGEMENT, VTConfig::NAME_TEXTURE, VTConfig::UNDEF));
    _name_mipmap = std::string(ini_config->GetValue(VTConfig::TEXTURE_MANAGEMENT, VTConfig::NAME_MIPMAP, VTConfig::UNDEF));
    _format_texture = VTConfig::which_texture_format(ini_config->GetValue(VTConfig::TEXTURE_MANAGEMENT, VTConfig::TEXTURE_FORMAT, VTConfig::UNDEF));
    _verbose = atoi(ini_config->GetValue(VTConfig::DEBUG, VTConfig::VERBOSE, VTConfig::UNDEF)) == 1;
}

uint16_t VTConfig::identify_depth()
{
    size_t fsize = 0;
    std::ifstream file;

    file.open(_name_mipmap + ".data", std::ifstream::in | std::ifstream::binary);

    if(!file.is_open())
    {
        throw std::runtime_error("Mipmap could not be read");
    }

    fsize = (size_t)file.tellg();
    file.seekg(0, std::ios::end);
    fsize = (size_t)file.tellg() - fsize;

    file.close();

    auto texel_count = (uint32_t)(size_t)fsize / get_byte_stride();

    size_t count_tiled = texel_count / _size_tile / _size_tile;

    size_t depth = QuadTree::get_depth_of_node(count_tiled - 1);

    return (uint16_t)depth;
}

uint32_t VTConfig::identify_size_index_texture() { return (uint32_t)std::pow(2, _depth_quadtree); }

scm::math::vec2ui VTConfig::calculate_size_physical_texture()
{
    size_t max_tex_byte_size = (size_t)_size_physical_texture * 1024 * 1024;

    // TODO: remove clutches

    uint32_t max_tex_layers = 16;
    //    GLint max_tex_layers;
    //    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_tex_layers);

    uint32_t max_tex_px_width_gl = 256000;
    //    GLint max_tex_px_width_gl;
    //    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_px_width_gl);

    size_t max_tex_px_width_custom = (size_t)std::pow(2, (size_t)std::log2(sqrt(max_tex_byte_size / get_byte_stride())));
    size_t max_tex_px_width = ((size_t)max_tex_px_width_gl < max_tex_px_width_custom ? (size_t)max_tex_px_width_gl : (size_t)max_tex_px_width_custom);

    size_t tex_tile_width = max_tex_px_width / _size_tile;
    size_t tex_px_width = tex_tile_width * _size_tile;
    size_t tex_byte_size = tex_px_width * tex_px_width * get_byte_stride();
    size_t layers = max_tex_byte_size / tex_byte_size;

    _phys_tex_px_width = (uint32_t)tex_px_width;
    _phys_tex_tile_width = (uint32_t)tex_tile_width;
    _phys_tex_layers = (uint16_t)layers < (uint16_t)max_tex_layers ? (uint16_t)layers : (uint16_t)max_tex_layers;

    return scm::math::vec2ui(_phys_tex_tile_width, _phys_tex_tile_width);
}
uint16_t VTConfig::get_size_tile() const { return _size_tile; }
uint16_t VTConfig::get_size_padding() const { return _size_padding; }
const std::string &VTConfig::get_name_texture() const { return _name_texture; }
const std::string &VTConfig::get_name_mipmap() const { return _name_mipmap; }
VTConfig::FORMAT_TEXTURE VTConfig::get_format_texture() const { return _format_texture; }
bool VTConfig::is_verbose() const { return _verbose; }
uint16_t VTConfig::get_depth_quadtree() const { return _depth_quadtree; }
uint32_t VTConfig::get_size_index_texture() const { return _size_index_texture; }
uint32_t VTConfig::get_size_physical_texture() const { return _size_physical_texture; }
uint32_t VTConfig::get_size_physical_update_throughput() const { return _size_physical_update_throughput; }
uint32_t VTConfig::get_phys_tex_px_width() const { return _phys_tex_px_width; }
uint32_t VTConfig::get_phys_tex_tile_width() const { return _phys_tex_tile_width; }
uint16_t VTConfig::get_phys_tex_layers() const { return _phys_tex_layers; }
}