//
// Created by moqe3167 on 30/05/18.
//

#ifndef LAMURE_UTILS_H
#define LAMURE_UTILS_H

//lamure
#include <lamure/types.h>
#include <lamure/ren/camera.h>
#include <lamure/ren/config.h>
#include <lamure/ren/policy.h>

//lamure vt
#include <lamure/vt/VTConfig.h>
#include <lamure/vt/ren/CutDatabase.h>
#include <lamure/vt/ren/CutUpdate.h>

//schism
#include <scm/core.h>
#include <scm/core/io/tools.h>
#include <scm/gl_util/primitives/primitives_fwd.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/font/font_face.h>
#include <scm/gl_util/font/text.h>
#include <scm/gl_util/font/text_renderer.h>
#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/primitives/wavefront_obj.h>

//boost
#include <boost/assign/list_of.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

namespace vis_utils
{
    char *get_cmd_option(char **begin, char **end, const std::string &option);

    bool cmd_option_exists(char **begin, char **end, const std::string &option);

    scm::math::mat4f load_matrix(const std::string& filename);

    std::string const strip_whitespace(std::string const& in_string);

    //checks for prefix AND removes it (+ whitespace) if it is found;
    //returns true, if prefix was found; else false
    bool parse_prefix(std::string &in_string, std::string const &prefix);

    void resolve_relative_path(std::string& base_path, std::string& relative_path);
}


#endif //LAMURE_UTILS_H
