//
// Created by towe2387 on 11/14/17.
//

#ifndef LAMURE_PREPROCESSOR_H
#define LAMURE_PREPROCESSOR_H

#include <ImageMagick-6/Magick++.h>
#include <algorithm>
#include <boost/filesystem/path.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <lamure/vt/QuadTree.h>
#include <lamure/vt/VTContext.h>
#include <lamure/vt/common.h>
#include <lamure/vt/ext/SimpleIni.h>
#include <lamure/vt/ext/morton.h>
#include <list>
#include <memory>
#include <mutex>

namespace vt
{
class Preprocessor
{
  public:
    explicit Preprocessor(VTContext &context);
    ~Preprocessor() = default;

    bool prepare_raster(const char *name_raster);
    bool prepare_mipmap();

  private:
    VTContext *_context;

    void extract_leaf_tile_range(uint32_t _thread_id);
    void stitch_tile_range(uint32_t _thread_id, size_t _node_start, size_t _node_end);
    void extract_leaf_tile_rows(uint32_t _thread_id);
    void read_dimensions(ifstream &ifs, size_t &dim_x, size_t &dim_y);
};
}

#endif // LAMURE_PREPROCESSOR_H
