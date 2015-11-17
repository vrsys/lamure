// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/bvh.h>

#include <limits>

#if WIN32
  #include <io.h>
#endif
 
#include <sys/stat.h>
#include <fcntl.h>

#include <lamure/ren/bvh_stream.h>


namespace lamure {
namespace ren {


Bvh::
Bvh()
: num_nodes_(0),
  fan_factor_(0),
  depth_(0),
  surfels_per_node_(0),
  size_of_surfel_(0),
  filename_(""),
  translation_(scm::math::vec3f(0.f)) {


} 

Bvh::
Bvh(const std::string& filename)
: num_nodes_(0),
  fan_factor_(0),
  depth_(0),
  surfels_per_node_(0),
  size_of_surfel_(0),
  filename_(""),
  translation_(scm::math::vec3f(0.f)) {

    std::string extension = filename.substr(filename.size()-3);
    if (extension.compare("bvh") == 0) {
       LoadBvhFile(filename);
    }
    else {
       throw std::runtime_error(
          "PLOD: Bvh::Invalid file extension encountered.");
    }

};

const node_t Bvh::
GetChildId(const node_t node_id, const node_t child_index) const {
    return node_id*fan_factor_ + 1 + child_index;
}

const node_t Bvh::
GetParentId(const node_t node_id) const {
    if (node_id == 0) return invalid_node_t;

    if (node_id % fan_factor_ == 0) {
        return node_id/fan_factor_ - 1;
    }
    else {
        return (node_id + fan_factor_ - (node_id % fan_factor_)) / fan_factor_ - 1;
    }
}

const node_t Bvh::
GetFirstNodeIdOfDepth(uint32_t depth) const {
    node_t id = 0;
    for (uint32_t i = 0; i < depth; ++i) {
        id += (node_t)pow((double)fan_factor_, (double)i);
    }

    return id;
}

const uint32_t Bvh::
GetLengthOfDepth(uint32_t depth) const {
    return pow((double)fan_factor_, (double)depth);
}

const uint32_t Bvh::
GetDepthOfNode(const node_t node_id) const {
    return (uint32_t)(std::log((node_id+1) * (fan_factor_-1)) / std::log(fan_factor_));
}

void Bvh::
LoadBvhFile(const std::string& filename) {

    filename_ = filename;

    BvhStream bvh_stream;
    bvh_stream.ReadBvh(filename, *this);
}


void Bvh::
WriteBvhFile(const std::string& filename) {
    
    filename_ = filename;

    BvhStream bvh_stream;
    bvh_stream.WriteBvh(filename, *this);

}

const scm::gl::boxf& Bvh::
GetBoundingBox(const node_t node_id) const {
    assert(node_id >= 0 && node_id < num_nodes_);
    return bounding_boxes_[node_id];
}

void Bvh::
SetBoundingBox(const lamure::node_t node_id, const scm::gl::boxf& bounding_box) {
    assert(node_id >= 0 && node_id < num_nodes_);
    while (bounding_boxes_.size() <= node_id) {
       bounding_boxes_.push_back(scm::gl::boxf());
    }
    bounding_boxes_[node_id] = bounding_box;
}

const scm::math::vec3f& Bvh::
GetCentroid(const node_t node_id) const {
    assert(node_id >= 0 && node_id < num_nodes_);
    return centroids_[node_id];
}

void Bvh::
SetCentroid(const lamure::node_t node_id, const scm::math::vec3f& centroid) {
    assert(node_id >= 0 && node_id < num_nodes_);
    while (centroids_.size() <= node_id) {
       centroids_.push_back(scm::math::vec3f(0.f, 0.f, 0.f));
    }
    centroids_[node_id] = centroid;
}

const float Bvh::
GetAvgSurfelRadius(const node_t node_id) const {
    assert(node_id >= 0 && node_id < num_nodes_);
    return avg_surfel_radii_[node_id];
}

void Bvh::
SetAvgSurfelRadius(const lamure::node_t node_id, const float radius) {
    assert(node_id >= 0 && node_id < num_nodes_);
    while (avg_surfel_radii_.size() <= node_id) {
       avg_surfel_radii_.push_back(0.f);
    }
    avg_surfel_radii_[node_id] = radius;
}

const Bvh::
NodeVisibility Bvh::GetVisibility(const node_t node_id) const {
    assert(node_id >= 0 && node_id < num_nodes_);
    return visibility_[node_id];
};

void Bvh::
SetVisibility(const node_t node_id, const Bvh::NodeVisibility visibility) {
    assert(node_id >= 0 && node_id < num_nodes_);
    while (visibility_.size() <= node_id) {
       visibility_.push_back(NodeVisibility::NODE_VISIBLE);
    }
    visibility_[node_id] = visibility;
};



} } // namespace lamure

