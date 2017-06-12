// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/io/format_xyz_all.h>

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>

namespace lamure
{
namespace pre
{

void format_xyzall::
read(const std::string &filename, surfel_callback_funtion callback)
{
    std::ifstream xyz_file_stream(filename);

    if (!xyz_file_stream.is_open())
        throw std::runtime_error("Unable to open input file: " +
            filename);

    std::string line;

    real pos[3];
    float norm[3];
    unsigned int color[3];
    real radius;

    xyz_file_stream.seekg(0, std::ios::end);
    std::streampos end_pos = xyz_file_stream.tellg();
    xyz_file_stream.seekg(0, std::ios::beg);
    uint8_t percent_processed = 0;

    while (getline(xyz_file_stream, line)) {

        uint8_t new_percent_processed = (xyz_file_stream.tellg() / float(end_pos)) * 100;
        if (percent_processed + 1 == new_percent_processed) {
            percent_processed = new_percent_processed;
            std::cout << "\r" << (int) percent_processed << "% processed" << std::flush;
        }

        std::stringstream sstream;

        sstream << line;

        sstream >> pos[0];
        sstream >> pos[1];
        sstream >> pos[2];
        sstream >> norm[0];
        sstream >> norm[1];
        sstream >> norm[2];
        sstream >> color[0];
        sstream >> color[1];
        sstream >> color[2];
        sstream >> radius;

        callback(surfel(vec3r(pos[0], pos[1], pos[2]),
                        vec3b(color[0], color[1], color[2]),
                        radius,
                        vec3f(norm[0], norm[1], norm[2])));
    }

    xyz_file_stream.close();
}

void format_xyzall::
write(const std::string &filename, buffer_callback_function callback)
{
    throw std::runtime_error("Not implemented yet!");
}

} // namespace pre
} // namespace lamure
