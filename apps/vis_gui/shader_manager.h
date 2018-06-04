//
// Created by moqe3167 on 30/05/18.
//

#ifndef LAMURE_SHADER_MANAGER_H
#define LAMURE_SHADER_MANAGER_H

#include "imgui_impl_glfw_gl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

//schism
#include <scm/core.h>
#include <scm/core/math.h>
#include <scm/core/io/tools.h>
#include <scm/core/pointer_types.h>
#include <scm/gl_core/gl_core_fwd.h>
#include <scm/gl_util/primitives/primitives_fwd.h>
#include <scm/core/platform/platform.h>
#include <scm/core/utilities/platform_warning_disable.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/font/font_face.h>
#include <scm/gl_util/font/text.h>
#include <scm/gl_util/font/text_renderer.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>
#include <scm/gl_util/data/imaging/texture_loader.h>

//boost
#include <boost/assign/list_of.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

//lamure
#include <lamure/types.h>
#include <lamure/ren/config.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/dataset.h>
#include <lamure/ren/policy.h>
#include <lamure/ren/controller.h>
#include <lamure/pvs/pvs_database.h>
#include <lamure/ren/ray.h>
#include <lamure/prov/aux.h>
#include <lamure/prov/octree.h>


#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <list>
#include <map>

#include "utils.h"

using namespace vis_utils;


class shader_manager {
public:
    shader_manager(const boost::shared_ptr<scm::gl::render_device> &device);

    void add(const std::string &shader_name, const std::string &vs_path, const std::string &fs_path, const std::string &gs_path = "");
    void add(const std::string &shader_name, bool keep_optional_shader_code, const std::string &vs_path, const std::string &fs_path, const std::string &gs_path = "");
    scm::gl::program_ptr get_program(const std::string & shader_name) const;

    double size();

private:
    struct shader_path {
        shader_path(const std::string &vs_path, const std::string &fs_path, const std::string &gs_path = "") :
                vs_path(vs_path), fs_path(fs_path), gs_path(gs_path) {}

        scm::gl::program_ptr shader_program;
        std::string vs_path{};
        std::string fs_path{};
        std::string gs_path{};
    };

    void load_shader(shader_path &path, const bool keep_optional_shader_code);
    bool read_shader(const std::string &path_string, std::string &shader_string, bool keep_optional_shader_code = false);


    std::map<std::string, shader_path> shaders;
    scm::shared_ptr<scm::gl::render_device> device_;
};


#endif //LAMURE_SHADER_MANAGER_H
