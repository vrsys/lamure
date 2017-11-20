// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>
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

    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << 
                     " input.obj output.xyz_all\n" <<
                     " (optional: use -fx and/or -fy to flip texture coords)" << std::endl;
        return -1;
    }

    sampler sampler;
    if (!sampler.load(argv[1]))
        return -1;

    if (!sampler.SampleMesh(argv[2]), 
      cmd_option_exists(argv, argv+argc, "-fx"), 
      cmd_option_exists(argv, argv+argc, "-fy"))
        return -1;

    return 0;
}

