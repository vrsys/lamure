//
// Created by moqe3167 on 20/11/17.
//

#include "TileStitcher.h"


TileStitcher::TileStitcher(std::string const &input_dir, std::string const &output_file) {
    this -> in_dir += input_dir;
    out_file.open(output_file, std::ios::out | std::ios::binary);

    // TODO read somewhere somehow the information about how much tiles in x direction we have
    this -> tiles_per_row = 8;
}

TileStitcher::~TileStitcher() = default;

long TileStitcher::absolute_byte_pos(int tile_height,
                                     int tile_width,
                                     int index_x,
                                     int index_y,
                                     int tile_row) {
    return COLOR_DEPTH * tile_width * (tiles_per_row * (tile_height * index_y + tile_row) + index_x);
}

void TileStitcher::swap_red_blue_32(BYTE *byte_row, const BYTE* line_size) {
    for(BYTE* pixel = byte_row; pixel < line_size; pixel += COLOR_DEPTH) {
        pixel[0] ^= pixel[2];
        pixel[2] ^= pixel[0];
        pixel[0] ^= pixel[2];
    }
}

void TileStitcher::swap_red_blue_32(FIBITMAP *tile) {
    const unsigned height   = FreeImage_GetHeight(tile);
    const unsigned pitch    = FreeImage_GetPitch(tile);
    const unsigned lineSize = FreeImage_GetLine(tile);

    BYTE* line = FreeImage_GetBits(tile);
    for(unsigned y = 0; y < height; ++y, line += pitch) {
        swap_red_blue_32(line, line + lineSize);
    }
}

bool TileStitcher::stitch() {
    std::vector<std::string> tile_index;
    int tile_index_x, tile_index_y;

    // Go through each tile in the specified folder
    for(auto& file : fs::directory_iterator(in_dir)) {
        // Get and split filename
        auto file_path = file.path().string();
        auto file_stem = file.path().stem().string();
        boost::split(tile_index, file_stem, boost::is_any_of("_"));

        tile_index_x = std::stoi(tile_index[0]);
        tile_index_y = std::stoi(tile_index[1]);

        FIBITMAP *tile  = load_and_convert_tile(file_path);

        int tile_width  = FreeImage_GetWidth(tile);
        int tile_height = FreeImage_GetHeight(tile);

        for (int y = 0; y <= tile_height - 1; y++) {
            // GetScanLine reads a line of data from bottom to top
            auto *tile_row = FreeImage_GetScanLine(tile, y);

            long abs_byte_pos = absolute_byte_pos(
                    tile_height,
                    tile_width,
                    tile_index_x,
                    tile_index_y,
                    tile_width - y - 1); // scanline bottom up => tile_row must go from 255 to 0

            out_file.seekp(abs_byte_pos);
            auto *byte_array = (char *) tile_row;
            out_file.write(byte_array, tile_width * COLOR_DEPTH);
        }

        FreeImage_Unload(tile);
    }

    out_file.close();
    return true;
}

FIBITMAP* TileStitcher::load_and_convert_tile(const std::string &file_path) {
    // Find file format and load tile in cache
    FREE_IMAGE_FORMAT format = FreeImage_GetFileType(file_path.c_str());
    FIBITMAP* tile = FreeImage_Load(format, file_path.c_str());

    // Convert tile in 32 bit representation for RGBA und swap red and blue channel
    tile = FreeImage_ConvertTo32Bits(tile);
    swap_red_blue_32(tile);

    return tile;
}


