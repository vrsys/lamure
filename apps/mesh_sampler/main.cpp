// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>
#include <string>
#include "sampler.h"

char* get_cmd_option(char** begin, char** end, const std::string & option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

int main(int argc, char **argv)
{

#ifdef USE_WEDGE_NORMALS
    std::cout << "Built with wedge normals support." << std::endl;
#endif

    std::string const attribute_texture_suffix_option = "-att_suffix";

    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << 
                     " input.obj output.xyz_all\n" <<
                     " (optional: use -fx and/or -fy to flip texture coords)\n" <<
                     " (optional: use " << attribute_texture_suffix_option << " to sample potential attribute\n" << 
                     "            -> creates xyz_rgba_all with attribute in alpha channel\n" 
                    << std::endl;
        return -1;
    }

    std::string attribute_texture_suffix = "";

    if(cmd_option_exists(argv, argv+argc, attribute_texture_suffix_option.c_str() )) {
        attribute_texture_suffix = get_cmd_option(argv, argv+argc, attribute_texture_suffix_option.c_str() );
    }

    sampler sampler;
    if (!sampler.load(argv[1], attribute_texture_suffix))
        return -1;



    bool sampling_successful = sampler.SampleMesh(argv[2], cmd_option_exists(argv, argv+argc, "-fx"), cmd_option_exists(argv, argv+argc, "-fy") );
    if (!sampling_successful) {
        return -1;
    }

    return 0;
}

