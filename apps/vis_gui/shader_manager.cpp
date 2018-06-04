//
// Created by moqe3167 on 30/05/18.
//

#include "shader_manager.h"

shader_manager::shader_manager(const boost::shared_ptr<scm::gl::render_device> &device) : device_(device) {}

void shader_manager::add(const std::string &shader_name,
                         const std::string &vs_path, const std::string &fs_path, const std::string &gs_path) {
    add(shader_name, false, vs_path, fs_path, gs_path);
}

void shader_manager::add(const std::string &shader_name, const bool keep_optional_shader_code,
                         const std::string &vs_path, const std::string &fs_path, const std::string &gs_path) {
    shader_path path(vs_path, fs_path, gs_path);
    load_shader(path, keep_optional_shader_code);
    shaders.insert({shader_name, path});
}

void shader_manager::load_shader(shader_path &shader, const bool keep_optional_shader_code) {
    std::string shader_root_path = LAMURE_SHADERS_DIR;
    std::string vs_source;
    std::string fs_source;
    std::string gs_source;


    if (shader.gs_path.empty()) {

        if (   !read_shader(shader_root_path + shader.vs_path, vs_source, keep_optional_shader_code)
            || !read_shader(shader_root_path + shader.fs_path, fs_source, keep_optional_shader_code)) {
            std::cout << "error reading shader files "
                      << shader.vs_path << " and " << shader.fs_path << std::endl;
            exit(1);
        }

        shader.shader_program = device_->create_program(
                boost::assign::list_of
                        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vs_source))
                        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, fs_source)));
    }
    else {

        if (   !read_shader(shader_root_path + shader.vs_path, vs_source, keep_optional_shader_code)
            || !read_shader(shader_root_path + shader.fs_path, fs_source, keep_optional_shader_code)
            || !read_shader(shader_root_path + shader.gs_path, gs_source, keep_optional_shader_code)) {
            std::cout << "error reading shader files "
                      << shader.vs_path << ", " << shader.fs_path << " and " << shader.gs_path << std::endl;
            exit(1);
        }

        shader.shader_program = device_->create_program(
                boost::assign::list_of
                        (device_->create_shader(scm::gl::STAGE_VERTEX_SHADER, vs_source))
                        (device_->create_shader(scm::gl::STAGE_GEOMETRY_SHADER, gs_source))
                        (device_->create_shader(scm::gl::STAGE_FRAGMENT_SHADER, fs_source)));
    }


    if (!shader.shader_program) {
        std::cout << "error creating shader "
                  << shader.vs_path.substr(0, shader.vs_path.length()-6) << " program" << std::endl;
        exit(1);
    }
}

scm::gl::program_ptr shader_manager::get_program(const std::string &shader_name) const {
    return shaders.at(shader_name).shader_program;
}

bool shader_manager::read_shader(std::string const& path_string,
                                 std::string& shader_string, bool keep_optional_shader_code) {
    if ( !boost::filesystem::exists( path_string ) ) {
        std::cout << "WARNING: File " << path_string << "does not exist." <<  std::endl;
        return false;
    }

    std::ifstream shader_source(path_string, std::ios::in);
    std::string line_buffer;

    std::string include_prefix("INCLUDE");

    std::string optional_begin_prefix("OPTIONAL_BEGIN");
    std::string optional_end_prefix("OPTIONAL_END");

    std::size_t slash_position = path_string.find_last_of("/\\");
    std::string const base_path =  path_string.substr(0,slash_position+1);

    bool disregard_code = false;

    while( std::getline(shader_source, line_buffer) ) {
        line_buffer = strip_whitespace(line_buffer);

        if( parse_prefix(line_buffer, include_prefix) ) {
            if(!disregard_code || keep_optional_shader_code) {
                std::string filename_string = line_buffer;
                read_shader(base_path+filename_string, shader_string);
            }
        } else if (parse_prefix(line_buffer, optional_begin_prefix)) {
            disregard_code = true;
        } else if (parse_prefix(line_buffer, optional_end_prefix)) {
            disregard_code = false;
        }
        else {
            if( (!disregard_code) || keep_optional_shader_code ) {
                shader_string += line_buffer+"\n";
            }
        }
    }

    return true;
}

double shader_manager::size() {
    return shaders.size();
}