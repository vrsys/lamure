// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_OCTREE_H_
#define PRE_OCTREE_H_

#include <lamure/types.h>

#include <lamure/pre/platform.h>
#include <scm/core/math.h>
#include <lamure/pre/surfel.h>

#include <string>
#include <fstream>
#include <cmath>
#include <iostream>
#include <vector>
#include <set>
#include <map>


namespace lamure {
namespace pre {


class PREPROCESSING_DLL octree_node {
public:
  octree_node()
    : idx_(0), child_mask_(0), child_idx_(0), min_(std::numeric_limits<double>::max()), max_(std::numeric_limits<double>::lowest()) {};
  octree_node(uint64_t _idx, uint32_t _child_mask, uint32_t _child_idx,
    const scm::math::vec3d& _min, const scm::math::vec3d& _max)
    : idx_(_idx), child_mask_(_child_mask), child_idx_(_child_idx), min_(_min), max_(_max) {};
  octree_node(uint64_t _idx, uint32_t _child_mask, uint32_t _child_idx,
    const scm::math::vec3d& _min, const scm::math::vec3d& _max, const surfel& surfel)
    : idx_(_idx), child_mask_(_child_mask), child_idx_(_child_idx), min_(_min), max_(_max), surfel_(surfel) {};
  ~octree_node() {};

  void set_idx(uint64_t _idx) { idx_ = _idx; };
  uint64_t get_idx() const { return idx_; };
  void set_child_mask(uint32_t _child_mask) { child_mask_ = _child_mask; };
  uint32_t get_child_mask() const { return child_mask_; };
  void set_child_idx(uint32_t _child_idx) { child_idx_ = _child_idx; };
  uint32_t get_child_idx() const { return child_idx_; };

  void set_min(const scm::math::vec3f& _min) { min_ = _min; };
  const scm::math::vec3d& get_min() const { return min_; };
  void set_max(const scm::math::vec3f& _max) { max_ = _max; };
  const scm::math::vec3d& get_max() const { return max_; };

  void set_surfel(const surfel& surfel) { surfel_ = surfel; };
  const surfel& get_surfel() const { return surfel_; };


protected:
  uint64_t idx_;
  uint32_t child_mask_; //char r_, g_, b_, child_mask_
  uint32_t child_idx_; //idx of first child
  scm::math::vec3d min_;
  scm::math::vec3d max_;

  surfel surfel_; //representative surfel for regularization
};

class PREPROCESSING_DLL octree {
public:
                      octree();
  virtual             ~octree();

  void                regularize(std::vector<surfel>& _input_points, double _min_voxel_edge_length, std::vector<surfel>& _output_points);
  uint64_t            query(const scm::math::vec3f& _pos);

  uint64_t            get_child_id(uint64_t node_id, uint32_t child_index);
  uint64_t            get_parent_id(uint64_t node_id);
  uint64_t            get_num_nodes() const;
  const octree_node&  get_node(uint64_t _node_id);
  void                add_node(const octree_node& _node);
  
  uint32_t            get_depth() const;
  void                set_depth(uint32_t _depth);

protected:


  std::vector<octree_node> nodes_;
  double min_voxel_edge_length_;
  uint32_t depth_;

private:

};


} } // namespace lamure


#endif // PRE_OCTREE_H_

