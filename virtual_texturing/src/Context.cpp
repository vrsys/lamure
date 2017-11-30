#include <lamure/vt/Context.h>
namespace vt
{
Context::Context(const char *path_config)
{
    _config = new CSimpleIniA(true, false, false);

    if(_config->LoadFile(path_config) < 0)
    {
        throw std::runtime_error("Configuration file parsing error");
    }

    read_config();
}

void Context::read_config()
{
    _size_tile = (uint16_t)atoi(_config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TILE_SIZE, Config::UNDEF));
    _name_texture = std::string(_config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_TEXTURE, Config::UNDEF));
    _name_mipmap = std::string(_config->GetValue(Config::TEXTURE_MANAGEMENT, Config::NAME_MIPMAP, Config::UNDEF));
    _opt_run_in_parallel = atoi(_config->GetValue(Config::TEXTURE_MANAGEMENT, Config::OPT_RUN_IN_PARALLEL, Config::UNDEF)) == 1;
    _opt_row_in_core = atoi(_config->GetValue(Config::TEXTURE_MANAGEMENT, Config::OPT_TILE_ROW_IN_CORE, Config::UNDEF)) == 1;
    _format_texture = Config::which_texture_format(_config->GetValue(Config::TEXTURE_MANAGEMENT, Config::TEXTURE_FORMAT, Config::UNDEF));
    _keep_intermediate_data = atoi(_config->GetValue(Config::DEBUG, Config::KEEP_INTERMEDIATE_DATA, Config::UNDEF)) == 1;
    _verbose = atoi(_config->GetValue(Config::DEBUG, Config::VERBOSE, Config::UNDEF)) == 1;
}

uint16_t Context::get_size_tile() const { return _size_tile; }
const std::string &Context::get_name_texture() const { return _name_texture; }
const std::string &Context::get_name_mipmap() const { return _name_mipmap; }
bool Context::is_opt_run_in_parallel() const { return _opt_run_in_parallel; }
bool Context::is_opt_row_in_core() const { return _opt_row_in_core; }
Config::FORMAT_TEXTURE Context::get_format_texture() const { return _format_texture; }
bool Context::is_keep_intermediate_data() const { return _keep_intermediate_data; }
bool Context::is_verbose() const { return _verbose; }

uint16_t Context::get_byte_stride() const { return Config::get_byte_stride(_format_texture); }
}