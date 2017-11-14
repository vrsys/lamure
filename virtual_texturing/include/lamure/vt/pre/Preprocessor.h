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

namespace vt {

class Preprocessor {
public:
  CSimpleIniA *config;

  explicit Preprocessor(const char *path_config);
  ~Preprocessor() = default;

  bool prepare_single_raster(const char *name_raster);
  bool prepare_mipmap();

private:

  void read_ppm_header(std::ifstream &_ifs, size_t &_dim_x, size_t &_dim_y);

  void write_tile_range_at_depth(uint32_t _thread_id, uint32_t _depth, size_t _node_start, size_t _node_end);

  void stitch_tile_range(uint32_t _thread_id, size_t _node_start, size_t _node_end);

  void write_stitched_tile(size_t _id, size_t _tile_size, std::array<Magick::Image, 4> &_child_imgs);
};

}

#endif //LAMURE_PREPROCESSOR_H
