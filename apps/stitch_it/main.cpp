#include <iostream>
#include "TileStitcher.h"
#include <ctime>

int main(int argc, char **argv) {
    std::string in_folder = "../../apps/stitch_it/8x8_montblanc";
    std::string out_file = "../../apps/stitch_it/montblanc.data";

    clock_t begin = clock();
    TileStitcher stitcher(in_folder, out_file);
    stitcher.stitch();
    clock_t end = clock();

    std::cout << "Runtime: "
              << double(end - begin) / CLOCKS_PER_SEC
              << " seconds."
              << std::endl;
    return 0;
}