#include <iostream>
#include "TileStitcher.h"

int main(int argc, char **argv) {
    auto begin = std::chrono::system_clock::now();
    auto begin_time = std::chrono::system_clock::to_time_t(begin);
    std::cout << "Start process at " << std::ctime(&begin_time) << std::endl;

#if 1
    std::string in_folder = "../../apps/stitch_it/8x8_montblanc_modified";
    std::string out_file = "../../apps/stitch_it/montblanc.data";
    TileStitcher stitcher(in_folder, out_file, 8, 8);
#else
    std::string in_folder = "/mnt/terabytes_of_textures/montblanc/jpeg_tiles";
    // std::string out_file  = "/mnt/terabytes_of_textures/montblanc/montblanc_full_w1202176_h304384.data";
    // TileStitcher stitcher(in_folder, out_file, 4696, 1189);
    std::string out_file  = "/mnt/terabytes_of_textures/montblanc/montblanc_100_100.data";
    TileStitcher stitcher(in_folder, out_file, 100, 100);
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