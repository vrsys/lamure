// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
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

#include <boost/tokenizer.hpp>


namespace lamure {
namespace pre {

void format_xyz_prov::
convert(const std::string& in_file, const std::string& out_file, bool xyz_rgb) {
  //only in-core

  std::ifstream input(in_file.c_str(), std::ios::in);
  if (!input.is_open()) {
    throw std::runtime_error("unable to open file " + in_file);
  }

  std::vector<prov_data> data;

  std::string line_buffer;
  uint64_t num_points = 0;

  while (getline(input, line_buffer)) {

    boost::char_separator<char> seperator(", ");
    boost::tokenizer<boost::char_separator<char>> tokens(line_buffer, seperator);

    prov_data v;

    uint32_t idx = 0;
    uint32_t i = 0;
    for (const auto& token : tokens) {
      if (!xyz_rgb || idx >= 6) { //ignore surfels
        if (i > num_prov_values_) {
          throw std::runtime_error("prov attribute exceeds size. increase prov_data::num_prov_values_. current value: " + num_prov_values_);
        }
        v.values_[i] = atof(token.c_str());
        ++i;
      }
      ++idx;
    }
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

