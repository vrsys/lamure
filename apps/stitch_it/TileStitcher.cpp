//
// Created by moqe3167 on 20/11/17.
//

#include "TileStitcher.h"
#include <iomanip>

TileStitcher::TileStitcher(
        std::string input_dir,
        std::string output_dir,
        std::string name,
        int tiles_per_row,
        int tiles_per_column) {
    this -> in_dir += input_dir;
    this -> out_dir += output_dir;

    this -> tiles_per_row    = (uint64_t) tiles_per_row;
    this -> tiles_per_column = (uint64_t) tiles_per_column;

    extract_tile_information();

    uint64_t height = standard_tile_height * (tiles_per_row - 1)    + last_column_height;
    uint64_t width  = standard_tile_width  * (tiles_per_column - 1) + last_row_width;
    std::string filename = out_dir.string() + name
                           + "_w" + std::to_string(width)
                           + "_h" + std::to_string(height)
                           + ".data";
    out_file.open(filename, std::ios::out | std::ios::binary);
}

TileStitcher::~TileStitcher() = default;


bool TileStitcher::stitch() {
    uint64_t counter = 0;

    auto begin_t = std::chrono::system_clock::now();

    for (int tile_index_y = 0; tile_index_y < tiles_per_column; ++tile_index_y) {
        for (int tile_index_x = 0; tile_index_x < tiles_per_row; ++tile_index_x) {
            auto file = get_path(tile_index_x, tile_index_y);

            log(begin_t, counter);

            FIBITMAP *tile  = load_and_convert_tile(file.string());

            int tile_width  = FreeImage_GetWidth(tile);
            int tile_height = FreeImage_GetHeight(tile);

            for (int y = 0; y <= tile_height - 1; y++) {
                // GetScanLine reads a line of data from bottom to top
                auto *tile_row = FreeImage_GetScanLine(tile, y);

                uint64_t abs_byte_pos = absolute_byte_pos(
                        tile_index_x,
                        tile_index_y,
                        tile_height - y - 1); // scanline bottom up => tile_row must go from 255 to 0

                out_file.seekp(abs_byte_pos);
                auto *byte_array = (char*) tile_row;
                out_file.write(byte_array, tile_width * COLOR_DEPTH);
            }

            FreeImage_Unload(tile);
        }
    }

    out_file.close();
    return true;
}


void TileStitcher::extract_tile_information() {
    fs::path first_tile_path;
    first_tile_path += get_path(0,0);
    auto first_tile_file = load_and_convert_tile(first_tile_path);

    standard_tile_width  = FreeImage_GetWidth(first_tile_file);
    standard_tile_height = FreeImage_GetHeight(first_tile_file);
    FreeImage_Unload(first_tile_file);

    fs::path last_tile_path;
    last_tile_path += get_path((int)(tiles_per_row - 1), (int)(tiles_per_column - 1));

    auto last_tile_file = load_and_convert_tile(last_tile_path);

    last_row_width     = FreeImage_GetWidth(last_tile_file);
    last_column_height = FreeImage_GetHeight(last_tile_file);
    FreeImage_Unload(last_tile_file);
}


std::experimental::filesystem::path TileStitcher::get_path(int x, int y) const {
    fs::path file;
    file += in_dir;
    file.append(std::to_string(x) + "_" + std::to_string(y) + ".jpeg");
    return file;
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


void TileStitcher::swap_red_blue_32(FIBITMAP *tile) {
    const unsigned height   = FreeImage_GetHeight(tile);
    const unsigned pitch    = FreeImage_GetPitch(tile);
    const unsigned lineSize = FreeImage_GetLine(tile);

    BYTE* line = FreeImage_GetBits(tile);
    for(unsigned y = 0; y < height; ++y, line += pitch) {
        swap_red_blue_32(line, line + lineSize);
    }
}


void TileStitcher::swap_red_blue_32(BYTE *tile_row, const BYTE *line_size) {
    for(BYTE* pixel = tile_row; pixel < line_size; pixel += COLOR_DEPTH) {
        pixel[0] ^= pixel[2];
        pixel[2] ^= pixel[0];
        pixel[0] ^= pixel[2];
    }
}


uint64_t TileStitcher::absolute_byte_pos(int index_x,
                                         int index_y,
                                         int tile_row) {
    return COLOR_DEPTH
           * (((tiles_per_row - 1) * standard_tile_width + last_row_width)
              * (standard_tile_height * index_y + tile_row)
              + index_x * standard_tile_width);
}


void TileStitcher::log(std::chrono::system_clock::time_point const& begin, uint64_t counter) {
    if (++counter % 100 == 0) {
        auto curr_t = std::chrono::system_clock::now();
        auto curr_time = std::chrono::system_clock::to_time_t(curr_t);

        std::chrono::duration<double> elapsed_minutes = (curr_t - begin) / 60.0f;
        std::string time_string = ctime(&curr_time);
        time_string = time_string.substr(0, time_string.size()-1);
        auto time_till_end = (tiles_per_column * tiles_per_row * elapsed_minutes) / counter;
        std::cout.setf(std::ios::fixed);
        std::cout << time_string
                  << "\t"
                  << std::setprecision(2) << elapsed_minutes.count() << " min"
                  << "\t"
                  << std::setprecision(2) << time_till_end.count() << " min"
                  << "\t"
                  << 100.0f * counter / (tiles_per_row * tiles_per_column) << "%"
                  << "\t"
                  << counter
                  << std::endl;
    }
}
