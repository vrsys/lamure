// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/raw_point_cloud.h>

namespace lamure
{

namespace ren
{

RawPointCloud::
RawPointCloud(const model_t model_id)
: model_id_(model_id),
  is_loaded_(false) {

}

RawPointCloud::
~RawPointCloud() {

}

const bool RawPointCloud::
load(const std::string& filename) {
    if (filename.substr(filename.find_last_of('.') + 1) != "xyz_all") {
        throw std::runtime_error(
            "RawPointCloud::Incompatible input file: " + filename);
    }

    std::ifstream xyz_file_stream(filename);

    if (!xyz_file_stream.is_open()) {
        throw std::runtime_error(
            "RawPointCloud::Unable to open input file: " + filename);
    }

    std::string line;

    float pos[3];
    float norm[3];
    uint32_t color[3];
    float radius;


    vec3f min = vec3f(std::numeric_limits<float>::max(),
                      std::numeric_limits<float>::max(),
                      std::numeric_limits<float>::max());
    vec3f max = vec3f(std::numeric_limits<float>::lowest(),
                      std::numeric_limits<float>::lowest(),
                      std::numeric_limits<float>::lowest());


    data_.clear();

    while (getline(xyz_file_stream, line)) {

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

        if (pos[0] < min[0]) min[0] = pos[0];
        if (pos[1] < min[1]) min[1] = pos[1];
        if (pos[2] < min[2]) min[2] = pos[2];
        if (pos[0] > max[0]) max[0] = pos[0];
        if (pos[1] > max[1]) max[1] = pos[1];
        if (pos[2] > max[2]) max[2] = pos[2];

        data_.push_back(serialized_surfel({
                            pos[0], pos[1], pos[2],
                            uint8_t(color[0]), uint8_t(color[1]), uint8_t(color[2]), 0,
                            radius/2.f,
                            norm[0], norm[1], norm[2]}));
    }

    xyz_file_stream.close();

    aabb_.min_vertex(min);
    aabb_.max_vertex(max);

    is_loaded_ = true;

    return true;
}


} // namespace ren

} // namespace lamure


