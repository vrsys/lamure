
#include <lamure/mesh/bvh.h>
#include <limits>
#include <queue>
#include <algorithm>
#include <iostream>

namespace lamure {
namespace mesh {

bvh::bvh(std::vector<triangle_t>& triangles)
: depth_(0) {

  create_hierarchy(triangles);

}

bvh::~bvh() {

}


const uint32_t bvh::
get_child_id(const uint32_t node_id, const uint32_t child_index) const {
    return node_id*2 + 1 + child_index;
}

const uint32_t bvh::
get_parent_id(const uint32_t node_id) const {
    if (node_id == 0) return 0;

    if (node_id % 2 == 0) {
        return node_id/2 - 1;
    }
    else {
        return (node_id + 2 - (node_id % 2)) / 2 - 1;
    }
}

const uint32_t bvh::
get_first_node_id_of_depth(uint32_t depth) const {
    uint32_t id = 0;
    for (uint32_t i = 0; i < depth; ++i) {
        id += (uint32_t)pow((double)2, (double)i);
    }

    return id;
}

const uint32_t bvh::
get_length_of_depth(uint32_t depth) const {
    return pow((double)2, (double)depth);
}

const uint32_t bvh::
get_depth_of_node(const uint32_t node_id) const {
    return (uint32_t)(std::log((node_id+1) * (2-1)) / std::log(2));
}


void bvh::create_hierarchy(std::vector<triangle_t>& triangles) {

  int64_t triangles_per_node = 1000;

  //Determine the bounding box of all tris ---root
  vec3f min(std::numeric_limits<float>::max());
  vec3f max(std::numeric_limits<float>::lowest());

  for (const auto& tri : triangles) { //for each
    auto centroid = tri.get_centroid();
    if (centroid.x < min.x) min.x = centroid.x;
    if (centroid.y < min.y) min.y = centroid.y;
    if (centroid.z < min.z) min.z = centroid.z;

    if (centroid.x > max.x) max.x = centroid.x;
    if (centroid.y > max.y) max.y = centroid.y;
    if (centroid.z > max.z) max.z = centroid.z;
  }

  std::queue<bvh_node> q;
  bvh_node root{
    0, //depth
    min, max,
    0, //begin
    triangles.size() //end
  };
  q.push(root);

  //remember the root
  nodes_.push_back(root);

  while (q.size() > 0) {
    bvh_node node = q.front(); //get the first element
    q.pop(); //remove the first element

    //Determine longest  axis of the node
    vec3f extend = node.max_ - node.min_;
    int32_t axis = 0; //0 = x axis, 1 = y axis, 2 = z axis
    if (extend.y > extend.x) {
      axis = 1;
      if (extend.z > extend.y) {
      	axis = 2;
      }
    }
    else if (extend.z > extend.x) {
      axis = 2;
    }

    //Sort all tris (of current node) by their centroid along the longest axis
    std::sort(triangles.begin()+node.begin_, triangles.begin()+node.end_, 
      [&](const triangle_t& a, const triangle_t& b) {

      	if (axis == 0) {
          return a.get_centroid().x > b.get_centroid().x;
        }
        else if (axis == 1) {
          return a.get_centroid().y > b.get_centroid().y;	
        }
        else if (axis == 2) {
          return a.get_centroid().z > b.get_centroid().z;
        }

        return true;
    });

    //determine split
    int64_t split_id = (node.begin_+node.end_)/2;

    //first create the children
    
    bvh_node left_child{
      node.depth_+1,
      node.min_, node.max_,
      node.begin_, split_id
    };
    
    bvh_node right_child{
      node.depth_+1,
      node.min_, node.max_,
      split_id, node.end_
    };

    //determine bounds of both children
    if (axis == 0) {
      left_child.max_.x = triangles[split_id].get_centroid().x;
      right_child.min_.x = triangles[split_id+1].get_centroid().x;
    }
    else if (axis == 1) {
      left_child.max_.y = triangles[split_id].get_centroid().y;
      right_child.min_.y = triangles[split_id+1].get_centroid().y; 
    }
    else if (axis == 2) {
      left_child.max_.z = triangles[split_id].get_centroid().z;
      right_child.min_.z = triangles[split_id+1].get_centroid().z;
    }

    if (left_child.end_-left_child.begin_ > triangles_per_node
      || right_child.end_-right_child.begin_ > triangles_per_node) {
        q.push(left_child);
        q.push(right_child);
    }

    nodes_.push_back(left_child);
    nodes_.push_back(right_child);

    depth_ = std::max(left_child.depth_, depth_);

  } //end of while

  std::cout << "construction done" << std::endl;
  std::cout << "hierarchy depth: " << depth_ << std::endl;
  std::cout << "number of nodes: " << nodes_.size() << std::endl;
  std::cout << "hierarchy min: " << nodes_[0].min_ << std::endl;
  std::cout << "hierarchy max: " << nodes_[0].max_ << std::endl;

}


}
}