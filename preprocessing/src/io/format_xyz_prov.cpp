// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/io/format_xyz_prov.h>

#include <lamure/pre/io/file.h>

#include <iostream>
#include <stdexcept>
#include <exception>

namespace lamure {
namespace pre {

void format_xyz_prov::
convert(const std::string& in_file, const std::string& out_file, bool xyz_rgb) {
  //only in-core

  std::ifstream input(in_file.c_str(), std::ios::in);
  if (!input.is_open()) {
    throw std::runtime_error("unable to open file " + in_file);
  }

  std::vector<prov> data;

  std::string line_buffer;
  uint64_t num_points = 0;

  while (getline(input, line_buffer)) {
    std::istringstream line(line_buffer);
    if (xyz_rgb) {
      //ignore surfels
      real pos[3];
      uint32_t color[3];

      line >> std::setprecision(LAMURE_STREAM_PRECISION) >> pos[0];
      line >> std::setprecision(LAMURE_STREAM_PRECISION) >> pos[1];
      line >> std::setprecision(LAMURE_STREAM_PRECISION) >> pos[2];
      line >> color[0];
      line >> color[1];
      line >> color[2];
    }
    prov v;
    line >> std::setprecision(LAMURE_STREAM_PRECISION) >> v.value_3_;
    line >> std::setprecision(LAMURE_STREAM_PRECISION) >> v.value_4_;
    line >> std::setprecision(LAMURE_STREAM_PRECISION) >> v.value_5_;
    line >> std::setprecision(LAMURE_STREAM_PRECISION) >> v.value_6_;
    data.push_back(v);
  }

  input.close();

  prov_file output;
  output.open(out_file, true);
  if (!output.is_open()) {
    throw std::runtime_error("unable to open file " + out_file);
  }

  output.write(&data, 0, 0, data.size());
  output.close();




}


}
}

