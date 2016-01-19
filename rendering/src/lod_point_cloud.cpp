// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/lod_point_cloud.h>

#include <scm/core/math.h>

namespace lamure
{

namespace ren
{

lod_point_cloud::
lod_point_cloud(const std::string& filename)
: model_id_(invalid_model_t),
  is_loaded_(false),
  bvh_(nullptr),
  transform_(scm::math::mat4f::identity()) {
    load(filename);
}

lod_point_cloud::
~lod_point_cloud() {
    if (bvh_ != nullptr) {
        delete bvh_;
        bvh_ = nullptr;
    }
}

void lod_point_cloud::
load(const std::string& filename) {

    std::string extension = filename.substr(filename.size()-3);
    
    if (extension.compare("kdn") == 0 || extension.compare("bvh") == 0) {
        bvh_ = new bvh(filename);
        is_loaded_ = true;
    }
    else {
        throw std::runtime_error(
            "lod_point_cloud::Incompatible input file: " + filename);
    }
    
}


} // namespace ren

} // namespace lamure

