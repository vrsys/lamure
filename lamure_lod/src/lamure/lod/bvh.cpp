// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/lod/bvh.h>

#include <limits>

#include <sys/stat.h>
#include <fcntl.h>

#if WIN32
  #include <io.h>
#endif

#include <lamure/lod/bvh_stream.h>
#include <lamure/math/bounding_box.h>

namespace lamure {
namespace lod {


bvh::
bvh()
: num_nodes_(0),
  fan_factor_(0),
  depth_(0),
  primitives_per_node_(0),
  size_of_primitive_(0),
  filename_(""),
  translation_(vec3r_t(0.0)),
  primitive_(primitive_type::POINTCLOUD) {


} 

bvh::
bvh(const std::string& filename)
: num_nodes_(0),
  fan_factor_(0),
  depth_(0),
  primitives_per_node_(0),
  size_of_primitive_(0),
  filename_(""),
  translation_(vec3r_t(0.0)) {

    std::string extension = filename.substr(filename.size()-3);
    if (extension.compare("bvh") == 0) {
       load_bvh_file(filename);
    }
    else {
       throw std::runtime_error(
          "lamure: bvh::Invalid file extension encountered.");
    }

};

const node_t bvh::
get_child_id(const node_t node_id, const node_t child_index) const {
    return node_id*fan_factor_ + 1 + child_index;
}

const node_t bvh::
get_parent_id(const node_t node_id) const {
    if (node_id == 0) return invalid_node_t;

    if (node_id % fan_factor_ == 0) {
        return node_id/fan_factor_ - 1;
    }
    else {
        return (node_id + fan_factor_ - (node_id % fan_factor_)) / fan_factor_ - 1;
    }
}

const node_t bvh::
get_first_node_id_of_depth(uint32_t depth) const {
    node_t id = 0;
    for (uint32_t i = 0; i < depth; ++i) {
        id += (node_t)pow((double)fan_factor_, (double)i);
    }

    return id;
}

const uint32_t bvh::
get_length_of_depth(uint32_t depth) const {
    return pow((double)fan_factor_, (double)depth);
}

const uint32_t bvh::
get_depth_of_node(const node_t node_id) const {
    return (uint32_t)(std::log((node_id+1) * (fan_factor_-1)) / std::log(fan_factor_));
}

void bvh::
load_bvh_file(const std::string& filename) {

    filename_ = filename;

    bvh_stream bvh_stream;
    bvh_stream.read_bvh(filename, *this);
}


void bvh::
write_bvh_file(const std::string& filename) {
    
    filename_ = filename;

    bvh_stream bvh_stream;
    bvh_stream.write_bvh(filename, *this);

}

const lamure::math::bounding_box_t& bvh::
get_bounding_box(const node_t node_id) const {
    ASSERT(node_id >= 0 && node_id < num_nodes_);
    return bounding_boxes_[node_id];
}

void bvh::
set_bounding_box(const node_t node_id, const lamure::math::bounding_box_t& bounding_box) {
    ASSERT(node_id >= 0 && node_id < num_nodes_);
    while (bounding_boxes_.size() <= node_id) {
       bounding_boxes_.push_back(lamure::math::bounding_box_t());
    }
    bounding_boxes_[node_id] = bounding_box;
}

const vec3r_t& bvh::
get_centroid(const node_t node_id) const {
    ASSERT(node_id >= 0 && node_id < num_nodes_);
    return centroids_[node_id];
}

void bvh::
set_centroid(const node_t node_id, const vec3r_t& centroid) {
    ASSERT(node_id >= 0 && node_id < num_nodes_);
    while (centroids_.size() <= node_id) {
       centroids_.push_back(vec3r_t(0.0, 0.0, 0.0));
    }
    centroids_[node_id] = centroid;
}

const float bvh::
get_avg_primitive_extent(const node_t node_id) const {
    ASSERT(node_id >= 0 && node_id < num_nodes_);
    return avg_primitive_extent_[node_id];
}

void bvh::
set_avg_primitive_extent(const node_t node_id, const float radius) {
    ASSERT(node_id >= 0 && node_id < num_nodes_);
    while (avg_primitive_extent_.size() <= node_id) {
       avg_primitive_extent_.push_back(0.f);
    }
    avg_primitive_extent_[node_id] = radius;
}

const bvh::
node_visibility bvh::get_visibility(const node_t node_id) const {
    ASSERT(node_id >= 0 && node_id < num_nodes_);
    return visibility_[node_id];
};

void bvh::
set_visibility(const node_t node_id, const bvh::node_visibility visibility) {
    ASSERT(node_id >= 0 && node_id < num_nodes_);
    while (visibility_.size() <= node_id) {
       visibility_.push_back(node_visibility::NODE_VISIBLE);
    }
    visibility_[node_id] = visibility;
};



} } // namespace lamure

