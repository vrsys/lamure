// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>
#include "TileStitcher.h"

int main(int argc, char **argv) {
    auto begin = std::chrono::system_clock::now();
    auto begin_time = std::chrono::system_clock::to_time_t(begin);
    std::cout << "Start process at " << std::ctime(&begin_time) << std::endl;

#if 0
    std::string in_folder  = "../../apps/stitch_it/8x8_montblanc_modified";
    std::string out_folder = "../../apps/stitch_it/";
    std::string file_name  = "mountblanc";
    TileStitcher stitcher(in_folder, out_folder, file_name, 8, 8);
#else
    std::string in_folder = "/mnt/terabytes_of_textures/montblanc/jpeg_tiles";
    std::string out_folder = "/mnt/terabytes_of_textures/montblanc";
    std::string file_name  = "montblanc";
    TileStitcher stitcher(in_folder, out_folder, file_name, 4696, 1189);

//    std::string out_folder = "/mnt/terabytes_of_textures/montblanc";
//    std::string file_name  = "mountblanc";
//    TileStitcher stitcher(in_folder, out_folder, file_name, 50, 50);
#endif

    stitcher.stitch();

    auto end = std::chrono::system_clock::now();
    auto end_time = std::chrono::system_clock::to_time_t(end);
    std::chrono::duration<double> elapsed_seconds = end-begin;

    std::cout << "End process at "
              << std::ctime(&end_time) << "\n"
              << "Runtime: "
              << elapsed_seconds.count()
              << " seconds."
              << std::endl;
    return 0;
}