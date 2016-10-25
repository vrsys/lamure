// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/lod/dataset.h>

namespace lamure {
namespace lod {

dataset::
dataset(const std::string& filename)
: model_id_(invalid_model_t),
  is_loaded_(false),
  bvh_(nullptr),
  transform_(mat4f_t::identity()) {
    load(filename);
}

dataset::
~dataset() {
    if (bvh_ != nullptr) {
        delete bvh_;
        bvh_ = nullptr;
    }
}

void dataset::
load(const std::string& filename) {

    std::string extension = filename.substr(filename.size()-3);
    
    if (extension.compare("bvh") == 0) {
        bvh_ = new bvh(filename);
        is_loaded_ = true;
    }
    else {
        throw std::runtime_error(
            "lamure: dataset::Incompatible input file: " + filename);
    }
    
}


} // namespace lod

} // namespace lamure


