//
// Created by towe2387 on 11/14/17.
//

#ifndef LAMURE_PREPROCESSOR_H
#define LAMURE_PREPROCESSOR_H

#include <iostream>
#include <list>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <memory>
#include <ImageMagick-6/Magick++.h>
#include <boost/filesystem/path.hpp>
#include <mutex>
#include <lamure/vt/ext/SimpleIni.h>
#include <lamure/vt/QuadTree.h>
#include <lamure/vt/common.h>
#include <lamure/vt/ext/morton.h>

namespace vt
{

class Preprocessor
{
public:
    CSimpleIniA *config;

    explicit Preprocessor(const char *path_config);
    ~Preprocessor() = default;

    bool prepare_single_raster(const char *name_raster);
    bool prepare_mipmap();

private:

    void extract_leaf_tile_range(uint32_t _thread_id);
    void stitch_tile_range(uint32_t _thread_id, size_t _node_start, size_t _node_end);
    void write_stitched_tile(size_t id, size_t tile_size, std::array<Magick::Image, 4> &child_imgs);
    void extract_leaf_tile_rows(uint32_t _thread_id);
    void read_dimensions(ifstream &ifs, size_t &dim_x, size_t &dim_y);
    uint8_t get_byte_stride(Config::FORMAT_TEXTURE _format_texture);
};

}

#endif //LAMURE_PREPROCESSOR_H
