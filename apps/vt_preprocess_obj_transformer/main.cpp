// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>
#include <algorithm>
#include <fstream>
#include <map>

#include <scm/core.h>
#include <scm/core/math.h>
#include "preprocess_obj_transformer.h"


char* get_cmd_option(char** begin, char** end, const std::string & option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

int main(int argc, char *argv[]) {

    if (argc == 1 ||
            !cmd_option_exists(argv, argv+argc, "-obj") ||
            !cmd_option_exists(argv, argv+argc, "-atlas")) {
      std::cout << "Usage: " << argv[0]
                << " -obj <input_mesh.obj> -atlas <preprocessed_atlas.atlas>"
                << std::endl
                <<"INFO: argv[0] " << std::endl ;
      return 0;
    }

    std::string in_obj_file   = get_cmd_option(argv, argv+argc, "-obj");
    std::string in_atlas_file = get_cmd_option(argv, argv+argc, "-atlas");

    auto *obj_transformer = new preprocess_obj_transformer(in_obj_file, in_atlas_file);

    obj_transformer->start();

    return 0;
}



