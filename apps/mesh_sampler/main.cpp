// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>
#include "Sampler.h"

int main(int argc, char **argv)
{

    //auto tex_norm = [](float c)->float { float o = fmod(c,1); return (o < 0.f)? 1.f+o:o; };
    
/*
    int w = 5;


    auto tex_p = [&](int c)->int { 
        //return (c >= 0) ? c % w : (w-abs(c%w)) % w; 
        return (c >= 0) ? c % w : (w+(c%w)) % w; 
    
    
    };


    std::cout << tex_p(5) << std::endl;
    std::cout << tex_p(0) << std::endl;
    std::cout << tex_p(3) << std::endl;
    std::cout << tex_p(-1) << std::endl;
    std::cout << tex_p(-2) << std::endl;
    std::cout << tex_p(-3) << std::endl;
    std::cout << tex_p(-4) << std::endl;
    std::cout << tex_p(-5) << std::endl;
    std::cout << tex_p(-6) << std::endl;
    std::cout << tex_p(-7) << std::endl;
    std::cout << tex_p(-8) << std::endl;
    std::cout << tex_p(-9) << std::endl;
    std::cout << tex_p(-10) << std::endl;
    std::cout << tex_p(-11) << std::endl;
    return 0;
*/
#ifdef USE_WEDGE_NORMALS
    std::cout << "Built with wedge normals support." << std::endl;
#endif

    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << 
                     " input.obj output.xyz_all" << std::endl;
        return -1;
    }

    Sampler sampler;
    if (!sampler.Load(argv[1]))
        return -1;

    if (!sampler.SampleMesh(argv[2]))
        return -1;

    return 0;
}

