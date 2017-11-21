//
// Created by moqe3167 on 20/11/17.
//

#ifndef LAMURE_TILESTITCHER_H
#define LAMURE_TILESTITCHER_H

#include <iostream>
#include <fstream>
#include <FreeImage.h>
#include <FreeImagePlus.h>
#include <experimental/filesystem>
#include <boost/algorithm/string.hpp> // used for splitting strings
namespace fs = std::experimental::filesystem;

class TileStitcher {
public:
    TileStitcher(std::string const& input_dir, std::string const& output_file);
    virtual ~TileStitcher();

    bool stitch();

private:
    static const int COLOR_DEPTH = 4;
    int tiles_per_row;

    fs::path in_dir;
    std::ofstream out_file;

    /***
     * Computes the absolute byte position of a tile row in a canvas.
     * @param tile_height Height in pixel of a given tile
     * @param tile_width Width in pixel of a given tile
     * @param index_x Tile index x
     * @param index_y Tile index y
     * @param tile_row Row index from a given tile
     * @param color_depth Color depth (e.g. RGBA32 = 4) for each pixel
     * @return Absolute byte position in
    */
    long absolute_byte_pos(int tile_height,
                           int tile_width,
                           int index_x,
                           int index_y,
                           int tile_row = 0);

    /**
     * Swaps the red and blue color channels in an array.
     * Depending on the system, FreeImage uses either BGRA or RGBA color order. To switch between them, tis method
     * swaps the first and the third values in the stream.
     * @param byte_row Pointer to the data
     * @param line_size Pointer to the end of the line
    */
    void swap_red_blue_32(BYTE *byte_row, const BYTE* line_size);
    void swap_red_blue_32(FIBITMAP* tile);

    FIBITMAP * load_and_convert_tile(const std::string &file_path);
};


#endif //LAMURE_TILESTITCHER_H
