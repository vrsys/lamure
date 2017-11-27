//
// Created by moqe3167 on 20/11/17.
//

#ifndef LAMURE_TILESTITCHER_H
#define LAMURE_TILESTITCHER_H

#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>

#include <FreeImage.h>
#include <FreeImagePlus.h>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

/**
 * TileStitcher combines several smaller images (tiles) to one big raw data picture.
 *
 * The single tiles has to be in the format <tile_index_x>_<tile_index_y>.jpeg saved in the passed @param in_dir.
 * The Algorithm assumes that the tiles where created from the top left to the bottom right corner,
 * so that the first tile 0_0.jpeg has the standard tile size of all other tiles except the tiles
 * in the last row. For these tiles, the first "last row tile" is loaded and examined.
 * The corresponding width value is stored in @param last_row_width and is to be assumed to be equal
 * for each other "last row tile".
 */
class TileStitcher {
public:
    TileStitcher(
            std::string input_dir,
            std::string output_dir,
            std::string name,
            int tiles_per_row,
            int tiles_per_column);
    virtual ~TileStitcher();

    /**
     * Main function of this class.
     * It stitches all files in the given directory and saves the output file.
     * @return Returns true if everything is stiched without an error
     */
    bool stitch();

private:
    //
    const uint64_t COLOR_DEPTH = 4;

    // Standard sizes of a tile. Determined by the first 0_0 tile.
    uint64_t standard_tile_width;
    uint64_t standard_tile_height;

    // Number of tiles in each direction
    uint64_t tiles_per_row;
    uint64_t tiles_per_column;

    // Width of the last tile row for images which are not equally tiled
    // (e.g. each tile is 256x256 but last row tiles are 200x256)
    uint64_t last_row_width;
    uint64_t last_column_height;

    // In- and output members
    fs::path in_dir;
    fs::path out_dir;
    std::ofstream out_file;

    /***
     * Computes the absolute byte position of a tile row in a canvas.
     * @param index_x Tile index x
     * @param index_y Tile index y
     * @param tile_row Row index from a tile
     * @param color_depth Color depth (e.g. RGBA32 = 4) for each pixel
     * @return Absolute byte position in the output file
    */
    uint64_t absolute_byte_pos(int index_x,
                               int index_y,
                               int tile_row);

    /**
     * Swaps the red and blue color channels in a tile.
     * Depending on the system, FreeImage uses either BGRA or RGBA color order.
     * To switch between them, this method swaps the first and the third values in the stream.
     * @param tile
     */
    void swap_red_blue_32(FIBITMAP* tile);

    /**
     * Swaps the red and blue color channel in a tile row.
     * @param tile_row Pointer to the row data
     * @param line_size Pointer to the end of the line
    */
    void swap_red_blue_32(BYTE *tile_row, const BYTE *line_size);

    /**
     * Loads a tile to memory, converts it to RGBA
     * and swaps the red and blue channel if necessary.
     * @param file_path Path to the file.
     * @return Tile file
     */
    FIBITMAP* load_and_convert_tile(std::string const& file_path);

    /**
     * Creates from the passed tile coordinates the corresponding path to the tile.
     * @param x X coordinate of a tile.
     * @param y Y coordinate of a tile.
     * @return Path to the file.
     */
    std::experimental::filesystem::path get_path(int x, int y) const;

    /**
     * Extracts further information about the tiles:
     * * Standard width and height of a tile
     * * width of the last tile row
     * and saves them in the corresponding member variable.
     */
    void extract_tile_information();

    /**
     * Logs in a frequent interval:
     * * The time when the log occurs
     * * The elapsed time in minutes
     * * An estimation how long the process will take
     * * The archived percentage so far
     * * The number of processed tiles
     * @param begin Time point when the process begun
     * @param counter Number of processed tiles so far
     */
    void log(std::chrono::system_clock::time_point const& begin, uint64_t &counter);
};

#endif //LAMURE_TILESTITCHER_H
