#ifndef LAMURE_CONTEXT_H
#define LAMURE_CONTEXT_H

#include <lamure/vt/ext/SimpleIni.h>
#include "common.h"
namespace vt
{
class Context
{
public:
    explicit Context(const char *path_config);
    ~Context() = default;

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

    uint16_t _size_tile;
    std::string _name_texture;
    std::string _name_mipmap;
    bool _opt_run_in_parallel;
    bool _opt_row_in_core;
    Config::FORMAT_TEXTURE _format_texture;
    bool _keep_intermediate_data;
    bool _verbose;

    void read_config();
};
}

#endif //LAMURE_CONTEXT_H
