// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/prov/octree.h>
#include <lamure/prov/aux.h>

#include <limits>
#include <vector>
#include <stack>
#include <map>


namespace lamure {
namespace prov {


octree::
octree()
: min_num_points_per_node_(16), 
  depth_(0) {


} 

octree::
~octree() {

}


void octree::
create(std::vector<aux::sparse_point>& _points) {
  nodes_.clear(); 
  depth_ = 0;

  uint64_t num_points = _points.size();
  if (num_points < min_num_points_per_node_) {
    std::cout << "Too few points " << std::endl; exit(0);
  }

  scm::math::vec3f tree_min(std::numeric_limits<float>::max());
  scm::math::vec3f tree_max(std::numeric_limits<float>::lowest());

  for (const auto& point : _points) {
    tree_min.x = std::min(tree_min.x, point.pos_.x);
    tree_min.y = std::min(tree_min.y, point.pos_.y);
    tree_min.z = std::min(tree_min.z, point.pos_.z);

    tree_max.x = std::max(tree_max.x, point.pos_.x);
    tree_max.y = std::max(tree_max.y, point.pos_.y);
    tree_max.z = std::max(tree_max.z, point.pos_.z);
  }
  
  //make the bounding box a cube
  auto tree_dim = tree_max - tree_min;
  float longest_axis = std::max(tree_dim.x, std::max(tree_dim.y, tree_dim.z));
  tree_max = tree_min + scm::math::vec3f(longest_axis);

  
  struct auxiliary_node {
    uint32_t idx_;
    uint32_t depth_;
    uint64_t begin_;
    uint64_t end_;
    scm::math::vec3f min_;
    scm::math::vec3f max_;
  };

  uint32_t num_nodes = 0;
  std::stack<auxiliary_node> nodes_todo;
  nodes_todo.push(
    auxiliary_node{
      num_nodes++, 0, 0, num_points, tree_min, tree_max
    });

  std::map<uint64_t, octree_node> node_idx_map;

  while (!nodes_todo.empty()) {
    auxiliary_node node = nodes_todo.top();
    nodes_todo.pop();

    depth_ = std::max(depth_, node.depth_);

    //some termination criterion
    if (node.depth_ > 19 || node.end_-node.begin_ <= min_num_points_per_node_) {
      uint32_t child_mask = 0;
      node_idx_map[node.idx_] = octree_node(node.idx_, child_mask, 0, node.min_, node.max_);
      continue;
    }

    auto min = node.min_;
    auto max = node.max_;
    auto mid = 0.5f*(node.min_+node.max_);

    //sort all points between begin and end along x-axis, find midpoint
    std::sort(_points.begin()+node.begin_, _points.begin()+node.end_, 
      [](const aux::sparse_point& _l, const aux::sparse_point& _r) -> bool { 
        return _l.pos_.x < _r.pos_.x; 
      }
    );
    uint64_t mid_id_x = 0;
    for (uint64_t i = node.begin_; i < node.end_; ++i) {
      if (_points[i].pos_.x >= mid.x) { mid_id_x = i; break; }
    }
    
    //sort all points between begin and end along y-axis, find midpoints
    std::sort(_points.begin()+node.begin_, _points.begin()+mid_id_x, 
      [](const aux::sparse_point& _l, const aux::sparse_point& _r) -> bool { 
        return _l.pos_.y < _r.pos_.y; 
      }
    );
    uint64_t mid_id_y0 = 0;
    for (uint64_t i = node.begin_; i < mid_id_x; ++i) {
      if (_points[i].pos_.y >= mid.y) { mid_id_y0 = i; break; }
    }

    std::sort(_points.begin()+mid_id_x, _points.begin()+node.end_, 
      [](const aux::sparse_point& _l, const aux::sparse_point& _r) -> bool { 
        return _l.pos_.y < _r.pos_.y; 
      }
    );
    uint64_t mid_id_y1 = 0;
    for (uint64_t i = mid_id_x; i < node.end_; ++i) {
      if (_points[i].pos_.y >= mid.y) { mid_id_y1 = i; break; }
    }

    //sort all points between begin and end along z-axis, find midpoints
    std::sort(_points.begin()+node.begin_, _points.begin()+mid_id_y0, 
      [](const aux::sparse_point& _l, const aux::sparse_point& _r) -> bool { 
        return _l.pos_.z < _r.pos_.z; 
      }
    );
    uint64_t mid_id_z0 = 0;
    for (uint64_t i = node.begin_; i < mid_id_y0; ++i) {
      if (_points[i].pos_.z >= mid.z) { mid_id_z0 = i; break; }
    }

    std::sort(_points.begin()+mid_id_y0, _points.begin()+mid_id_x, 
      [](const aux::sparse_point& _l, const aux::sparse_point& _r) -> bool { 
        return _l.pos_.z < _r.pos_.z; 
      }
    );
    uint64_t mid_id_z1 = 0;
    for (uint64_t i = mid_id_y0; i < mid_id_x; ++i) {
      if (_points[i].pos_.z >= mid.z) { mid_id_z1 = i; break; }
    }
    
    std::sort(_points.begin()+mid_id_x, _points.begin()+mid_id_y1, 
      [](const aux::sparse_point& _l, const aux::sparse_point& _r) -> bool { 
        return _l.pos_.z < _r.pos_.z; 
      }
    );
    uint64_t mid_id_z2 = 0;
    for (uint64_t i = mid_id_x; i < mid_id_y1; ++i) {
      if (_points[i].pos_.z >= mid.z) { mid_id_z2 = i; break; }
    }
    
    std::sort(_points.begin()+mid_id_y1, _points.begin()+node.end_, 
      [](const aux::sparse_point& _l, const aux::sparse_point& _r) -> bool { 
        return _l.pos_.z < _r.pos_.z; 
      }
    );
    uint64_t mid_id_z3 = 0;
    for (uint64_t i = mid_id_y1; i < node.end_; ++i) {
      if (_points[i].pos_.z >= mid.z) { mid_id_z3 = i; break; }
    }


    std::vector<auxiliary_node> children;
    children.push_back(
      auxiliary_node{
        0, node.depth_+1,
        node.begin_, mid_id_z0,
        min, 
        mid,
      });
    children.push_back(
      auxiliary_node{
        0, node.depth_+1, 
        mid_id_x, mid_id_z2,
        scm::math::vec3f(mid.x, min.y, min.z),
        scm::math::vec3f(max.x, mid.y, mid.z)
      });
    children.push_back(
      auxiliary_node{
        0, node.depth_+1, 
        mid_id_y0, mid_id_z1,
        scm::math::vec3f(min.x, mid.y, min.z),
        scm::math::vec3f(mid.x, max.y, mid.z)
      });
    children.push_back(
      auxiliary_node{
        0, node.depth_+1, 
        mid_id_y1, mid_id_z3,
        scm::math::vec3f(mid.x, mid.y, min.z),
        scm::math::vec3f(max.x, max.y, mid.z)
      });
    children.push_back(
      auxiliary_node{
        0, node.depth_+1, 
        mid_id_z0, mid_id_y0,
        scm::math::vec3f(min.x, min.y, mid.z),
        scm::math::vec3f(mid.x, mid.y, max.z)
      });
    children.push_back(
      auxiliary_node{
        0, node.depth_+1, 
        mid_id_z2, mid_id_y1,
        scm::math::vec3f(mid.x, min.y, mid.z),
        scm::math::vec3f(max.x, mid.y, max.z)
      });
    children.push_back(
      auxiliary_node{
        0, node.depth_+1, 
        mid_id_z1, mid_id_x,
        scm::math::vec3f(min.x, mid.y, mid.z),
        scm::math::vec3f(mid.x, max.y, max.z)
      });
    children.push_back(
      auxiliary_node{
        0, node.depth_+1, 
        mid_id_z3, node.end_,
        mid,
        max,
      });

    uint32_t child_mask = 0;
    uint32_t child_idx = 0;

    //register valid children
    for (uint32_t i = 0; i < 8; ++i) {
      auto& child = children[i];
      bool register_child = true;
      if (child.end_ - child.begin_ == 0) {
        register_child = false;
      }
      if (register_child) {
        child.idx_ = num_nodes;
        nodes_todo.push(child);
        child_mask |= (1 << i);
        child_idx = child_idx == 0 ? num_nodes : child_idx;
        ++num_nodes;
      }
    }
    
    node_idx_map[node.idx_] = octree_node(node.idx_, child_mask, child_idx, node.min_, node.max_);

  }

  std::cout << "octree complete " << "depth: " << depth_ << " num nodes: " << num_nodes << std::endl;


}


} } // namespace lamure

