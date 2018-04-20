// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/io/format_xyz_bin.h>

#include <stdexcept>
#include <exception>

namespace lamure {
namespace pre {

void format_xyz_bin::
read(const std::string& filename, surfel_callback_funtion callback)
{
  std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("unable to open file " + filename);
  }

  file.seekg(std::ios::beg, std::ios::end);
  size_t file_size = file.tellg();
  file.seekg(std::ios::beg, std::ios::beg);
  size_t num_surfels = file_size / 32; //sizeof surfel in xyz_bin

  for (uint64_t i = 0; i < num_surfels; ++i) {
    
    vec3f pos;
    file.read((char*)&pos.x, 4);
    file.read((char*)&pos.y, 4);
    file.read((char*)&pos.z, 4);

    vec3b color; char alpha;
    file.read((char*)&color.x, 1);
    file.read((char*)&color.y, 1);
    file.read((char*)&color.z, 1);
    file.read((char*)&alpha, 1);

    float radius;
    file.read((char*)&radius, 4);

    vec3f normal;
    file.read((char*)&normal.x, 4);
    file.read((char*)&normal.y, 4);
    file.read((char*)&normal.z, 4);

    callback(surfel(vec3r(pos.x, pos.y, pos.z), color, (real)radius, normal));

  }

  file.close();
}

void format_xyz_bin::
write(const std::string& filename, buffer_callback_function callback)
{
  throw std::runtime_error("output for format .xyz_bin not implemented");
}

}
}

