
#include "kdtree.h"

uint32_t kdtree_t::invalid_node_id_t = std::numeric_limits<uint32_t>::max();

kdtree_t::
kdtree_t(const std::vector<indexed_triangle_t>& _triangles, uint32_t _tris_per_node)
: num_triangles_per_node_(_tris_per_node),
  fanout_factor_(2),
  depth_(0) {
  create(_triangles);
}


const uint32_t kdtree_t::
get_child_id(const uint32_t node_id, const uint32_t child_index) const {
    return node_id*fanout_factor_ + 1 + child_index;
}

const uint32_t kdtree_t::
get_parent_id(const uint32_t node_id) const {
    if (node_id == 0) return kdtree_t::invalid_node_id_t;

    if (node_id % fanout_factor_ == 0)
        return node_id/fanout_factor_ - 1;
    else
        return (node_id + fanout_factor_ - (node_id % fanout_factor_)) / fanout_factor_ - 1;
}

const uint32_t kdtree_t::
get_depth_of_node(const uint32_t node_id) const {
    return (uint32_t)(std::log((node_id+1) * (fanout_factor_-1)) / std::log(fanout_factor_));
}

const uint32_t kdtree_t::
get_length_of_depth(const uint32_t depth) const {
    return (uint32_t)(std::pow((double)fanout_factor_, (double)depth));
}


const uint32_t kdtree_t::
get_first_node_id_of_depth(const uint32_t depth) const {
    uint32_t id = 0;
    for (uint32_t i = 0; i < depth; ++i){
        id += (uint32_t)std::pow((double)fanout_factor_, (double)i);
    }

    return id;
}


void kdtree_t::
sort_indices(
  const kdtree_t::node_t& _node, 
  const std::vector<indexed_triangle_t>& _triangles, 
  std::vector<uint32_t>& _indices, 
  kdtree_t::axis_t _axis) {

    uint32_t num_tris_in_node = _node.end_ - _node.begin_;
    std::vector<uint32_t> temp_indices(num_tris_in_node, 0);

    const uint32_t hist_length = 2048;
    std::vector<signed long long> hist(hist_length * 3, 0);

    //fill histograms
    for (uint32_t s = _node.begin_; s < _node.end_; s++) {
        const indexed_triangle_t& tri = _triangles[_indices[s]];
        scm::math::vec3f centroid = tri.get_centroid();

        float fv = _axis == AXIS_X ? centroid.x : _axis == AXIS_Y ? centroid.y : centroid.z;
        uint32_t v = *(uint32_t*)(&fv);

        v ^= -int32_t(v >> 31) | 0x80000000;

        hist[(v & 0x7ff)]++;
        hist[hist_length + (v >> 11 & 0x7ff)]++;
        hist[hist_length + hist_length + (v >> 22)]++;
    }


    //scan histograms
    signed long long temp;
    signed long long k0 = 0, k1 = 0, k2 = 0;
    for (int k = 0; k < hist_length; k++) {
        temp = hist[k] + k0;
        hist[k] = k0 - 1;
        k0 = temp;

        temp = hist[hist_length + k] + k1;
        hist[hist_length + k] = k1 - 1;
        k1 = temp;

        temp = hist[hist_length + hist_length + k] + k2;
        hist[hist_length + hist_length + k] = k2 - 1;
        k2 = temp;
    }

    //pass 0
    for (uint32_t s = _node.begin_; s < _node.end_; s++) {
        const indexed_triangle_t& tri = _triangles[_indices[s]];
        scm::math::vec3f centroid = tri.get_centroid();

        float fv = _axis == AXIS_X ? centroid.x : _axis == AXIS_Y ? centroid.y : centroid.z;
        uint32_t v = *(uint32_t*)(&fv);


        v ^= -int32_t(v >> 31) | 0x80000000;
        size_t pos = ++hist[(v & 0x7ff)];

        temp_indices[pos] = _indices[s];
    }

    //pass 1
    for (uint32_t s = 0; s < num_tris_in_node; s++) {
        const indexed_triangle_t& tri = _triangles[temp_indices[s]];
        scm::math::vec3f centroid = tri.get_centroid();

        float fv = _axis == AXIS_X ? centroid.x : _axis == AXIS_Y ? centroid.y : centroid.z;
        uint32_t v = *(uint32_t*)(&fv);

        v ^= -int32_t(v >> 31) | 0x80000000;
        size_t pos = ++hist[hist_length + (v >> 11 & 0x7ff)];

        _indices[_node.begin_ + pos] = temp_indices[s];
    }

    //pass 2
    for (uint32_t s = _node.begin_; s < _node.end_; s++) {
        const indexed_triangle_t& tri = _triangles[_indices[s]];
        scm::math::vec3f centroid = tri.get_centroid();

        float fv = _axis == AXIS_X ? centroid.x : _axis == AXIS_Y ? centroid.y : centroid.z;
        uint32_t v = *(uint32_t*)(&fv);


        v ^= -int32_t(v >> 31) | 0x80000000;
        size_t pos = ++hist[hist_length + hist_length + (v >> 22)];

        temp_indices[pos] = _indices[s];
    }

    for (uint32_t i = 0; i < num_tris_in_node; ++i) {
      _indices[_node.begin_+i] = temp_indices[i];
    }

    //cleanup
    temp_indices.clear();
    hist.clear();

}



void kdtree_t::
create(const std::vector<indexed_triangle_t>& _triangles) {

  nodes_.clear();

  kdtree_t::node_t root;
  root.node_id_ = 0;
  root.min_ = scm::math::vec3f(std::numeric_limits<float>::max());
  root.max_ = scm::math::vec3f(std::numeric_limits<float>::lowest());
  root.begin_ = 0;
  root.end_ = _triangles.size();
  root.error_ = 0.f;


  for (unsigned int i = root.begin_; i < root.end_; ++i) {
    const indexed_triangle_t& tri = _triangles[i];

    if (tri.v0_.pos_.x < root.min_.x) root.min_.x = tri.v0_.pos_.x;
    if (tri.v0_.pos_.y < root.min_.y) root.min_.y = tri.v0_.pos_.y;
    if (tri.v0_.pos_.z < root.min_.z) root.min_.z = tri.v0_.pos_.z;
    if (tri.v0_.pos_.x > root.max_.x) root.max_.x = tri.v0_.pos_.x;
    if (tri.v0_.pos_.y > root.max_.y) root.max_.y = tri.v0_.pos_.y;
    if (tri.v0_.pos_.z > root.max_.z) root.max_.z = tri.v0_.pos_.z;

    if (tri.v1_.pos_.x < root.min_.x) root.min_.x = tri.v1_.pos_.x;
    if (tri.v1_.pos_.y < root.min_.y) root.min_.y = tri.v1_.pos_.y;
    if (tri.v1_.pos_.z < root.min_.z) root.min_.z = tri.v1_.pos_.z;
    if (tri.v1_.pos_.x > root.max_.x) root.max_.x = tri.v1_.pos_.x;
    if (tri.v1_.pos_.y > root.max_.y) root.max_.y = tri.v1_.pos_.y;
    if (tri.v1_.pos_.z > root.max_.z) root.max_.z = tri.v1_.pos_.z;

    if (tri.v2_.pos_.x < root.min_.x) root.min_.x = tri.v2_.pos_.x;
    if (tri.v2_.pos_.y < root.min_.y) root.min_.y = tri.v2_.pos_.y;
    if (tri.v2_.pos_.z < root.min_.z) root.min_.z = tri.v2_.pos_.z;
    if (tri.v2_.pos_.x > root.max_.x) root.max_.x = tri.v2_.pos_.x;
    if (tri.v2_.pos_.y > root.max_.y) root.max_.y = tri.v2_.pos_.y;
    if (tri.v2_.pos_.z > root.max_.z) root.max_.z = tri.v2_.pos_.z;
  }

  //predict tree properties
  depth_ = (uint32_t)(0.5f + std::log(_triangles.size() / num_triangles_per_node_) / std::log(fanout_factor_));
  uint32_t num_leafs = (uint32_t)(0.5f + exp(depth_*std::log(fanout_factor_)));
  num_triangles_per_node_ = (uint32_t)(ceil(float(_triangles.size()) / float(num_leafs)));

  indices_.clear();
  for (uint32_t i = 0; i < _triangles.size(); ++i) {
    indices_.push_back(i);
  }

  uint32_t num_nodes = 0;
  for (uint32_t i = 0; i <= depth_; ++i) {
    num_nodes += (uint32_t)pow((double)fanout_factor_, (double)i);
  }

  std::cout << "LOG: kdtree predicted num nodes: " << num_nodes << std::endl;
  std::cout << "LOG: kdtree predicted triangles per leaf: " << num_triangles_per_node_ << std::endl;

  std::queue<kdtree_t::node_t> nodes_todo;
  nodes_todo.push(root);

  bool show_progress = false;

  uint32_t num_nodes_done = 0;

  while (!nodes_todo.empty()) {

    if (show_progress) {
      //last::util::show_progress();
    }

    kdtree_t::node_t node = nodes_todo.front();
    nodes_todo.pop();

    nodes_.push_back(node);

    if (node.end_ - node.begin_ <= num_triangles_per_node_) {
      continue;
    }

    //determine longest axis
    float width = node.max_.x - node.min_.x;
    float height = node.max_.y - node.min_.y;
    float depth = node.max_.z - node.min_.z;
    axis_t axis = width > height ?
        (width > depth ? AXIS_X : AXIS_Z) :
        height > depth ? AXIS_Y : AXIS_Z;

    //sort between begin and end along axis
    //note: sort based on triangle centroids!
    sort_indices(node, _triangles, indices_, axis);

    //split node into equal blocks
    uint32_t num_tris_per_child = (node.end_ - node.begin_)/2;
    uint32_t id_first_right = indices_[node.begin_ + num_tris_per_child];

    const indexed_triangle_t& l = _triangles[id_first_right];
    scm::math::vec3f cl = l.get_centroid();
    const indexed_triangle_t& r = _triangles[id_first_right+1];
    scm::math::vec3f cr = r.get_centroid();

    kdtree_t::node_t left_child;
    kdtree_t::node_t right_child;

    left_child.node_id_ = get_child_id(node.node_id_, 0);
    right_child.node_id_ = get_child_id(node.node_id_, 1);

    left_child.max_ = node.max_;
    left_child.min_ = node.min_;
    right_child.max_ = node.max_;
    right_child.min_ = node.min_;

    left_child.begin_ = node.begin_;
    left_child.end_ = node.begin_ + num_tris_per_child;
    right_child.begin_ = left_child.end_;
    right_child.end_ = node.end_;

    left_child.error_ = 0.f;
    right_child.error_ = 0.f;

    bool adjust_bounds_of_inner_nodes = true;

    switch (axis) {
        case AXIS_X:
            if (adjust_bounds_of_inner_nodes) {
                left_child.max_.x = std::numeric_limits<float>::lowest();
                right_child.min_.x = std::numeric_limits<float>::max();
                for (uint32_t i = left_child.begin_; i < left_child.end_; ++i) {
                    const indexed_triangle_t& tri = _triangles[indices_[i]];
                    if (tri.v0_.pos_.x > left_child.max_.x) left_child.max_.x = tri.v0_.pos_.x;
                    if (tri.v1_.pos_.x > left_child.max_.x) left_child.max_.x = tri.v1_.pos_.x;
                    if (tri.v2_.pos_.x > left_child.max_.x) left_child.max_.x = tri.v2_.pos_.x;
                }
                for (uint32_t i = right_child.begin_; i < right_child.end_; ++i) {
                    const indexed_triangle_t& tri = _triangles[indices_[i]];
                    if (tri.v0_.pos_.x < right_child.min_.x) right_child.min_.x = tri.v0_.pos_.x;
                    if (tri.v1_.pos_.x < right_child.min_.x) right_child.min_.x = tri.v1_.pos_.x;
                    if (tri.v2_.pos_.x < right_child.min_.x) right_child.min_.x = tri.v2_.pos_.x;
                }
            }
            else {
                float split = (cr.x + cl.x)/2.0f;
                left_child.max_.x = split;
                right_child.min_.x = split;
            }
            break;

        case AXIS_Y:
            if (adjust_bounds_of_inner_nodes) {
                left_child.max_.y = std::numeric_limits<float>::lowest();
                right_child.min_.y = std::numeric_limits<float>::max();
                for (uint32_t i = left_child.begin_; i < left_child.end_; ++i) {
                    const indexed_triangle_t& tri = _triangles[indices_[i]];
                    if (tri.v0_.pos_.y > left_child.max_.y) left_child.max_.y = tri.v0_.pos_.y;
                    if (tri.v1_.pos_.y > left_child.max_.y) left_child.max_.y = tri.v1_.pos_.y;
                    if (tri.v2_.pos_.y > left_child.max_.y) left_child.max_.y = tri.v2_.pos_.y;
                }
                for (uint32_t i = right_child.begin_; i < right_child.end_; ++i) {
                    const indexed_triangle_t& tri = _triangles[indices_[i]];
                    if (tri.v0_.pos_.y < right_child.min_.y) right_child.min_.y = tri.v0_.pos_.y;
                    if (tri.v1_.pos_.y < right_child.min_.y) right_child.min_.y = tri.v1_.pos_.y;
                    if (tri.v2_.pos_.y < right_child.min_.y) right_child.min_.y = tri.v2_.pos_.y;
                }
            }
            else {
                float split = (cr.y + cl.y)/2.0f;
                left_child.max_.y = split;
                right_child.min_.y = split;
            }
            break;

        case AXIS_Z:
            if (adjust_bounds_of_inner_nodes) {
                left_child.max_.z = std::numeric_limits<float>::lowest();
                right_child.min_.z = std::numeric_limits<float>::max();
                for (uint32_t i = left_child.begin_; i < left_child.end_; ++i) {
                    const indexed_triangle_t& tri = _triangles[indices_[i]];
                    if (tri.v0_.pos_.z > left_child.max_.z) left_child.max_.z = tri.v0_.pos_.z;
                    if (tri.v1_.pos_.z > left_child.max_.z) left_child.max_.z = tri.v1_.pos_.z;
                    if (tri.v2_.pos_.z > left_child.max_.z) left_child.max_.z = tri.v2_.pos_.z;
                }
                for (uint32_t i = right_child.begin_; i < right_child.end_; ++i) {
                    const indexed_triangle_t& tri = _triangles[indices_[i]];
                    if (tri.v0_.pos_.z < right_child.min_.z) right_child.min_.z = tri.v0_.pos_.z;
                    if (tri.v1_.pos_.z < right_child.min_.z) right_child.min_.z = tri.v1_.pos_.z;
                    if (tri.v2_.pos_.z < right_child.min_.z) right_child.min_.z = tri.v2_.pos_.z;
                }
            }
            else {
                float split = (cr.z + cl.z)/2.0f;
                left_child.max_.z = split;
                right_child.min_.z = split;
            }
            break;

          default: break;
    }

    nodes_todo.push(left_child);
    nodes_todo.push(right_child);

  }
  if (show_progress) {
    std::cout << std::endl;
  }

  
  //store depth of tree
  depth_ = get_depth_of_node((uint32_t)nodes_.size()-1);
  num_nodes = (uint32_t)nodes_.size();

  std::cout << "LOG: kdtree depth: " << depth_ << std::endl;
  std::cout << "LOG: kdtree num nodes: " << nodes_.size() << std::endl;
  std::cout << "LOG: kdtree num leafs: " << get_length_of_depth(depth_) << std::endl;
  



}


