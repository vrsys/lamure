// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_pair_contraction.h>
#include <lamure/pre/surfel.h>
#include <set>
#include <functional>
#include <queue>
#include <deque>
#include <map>
#include <cmath>
#include <array>
#include <unordered_map>

// #define DEBUG
// #define ERROR_COLOR
// #define LEAF_REMOVAL
#define LIMIT_NEIGHBOURS

namespace lamure {

namespace pre {
struct quadric_t {
  quadric_t()
   :values{0,0,0,0,0,0,0,0,0,0}
   ,hess{0.0}
  {}
  quadric_t(vec4r const& hessian) 
   :values{hessian.x * hessian.x, hessian.y * hessian.x, hessian.z * hessian.x, hessian.w * hessian.x,
                                  hessian.y * hessian.y, hessian.z * hessian.y, hessian.w * hessian.y,
                                                         hessian.z * hessian.z, hessian.w * hessian.z,
                                                                                hessian.w * hessian.w}
   ,hess{hessian}
  {}

  real error2(vec3r const& p) const {
      return values[0] * p.x * p.x + 2.0 * values[1] * p.x * p.y  + 2.0 * values[2] * p.x * p.z + 2.0 * values[3] * p.x 
                                   +       values[4] * p.y * p.y  + 2.0 * values[5] * p.y * p.z + 2.0 * values[6] * p.y 
                                                                +         values[7] * p.z * p.z + 2.0 * values[8] * p.z 
                                                                                                +       values[9];  
  }
  real error(vec3r const& p) const {
    real a = dot(hess, vec4r{p, 1});
    return a * a;
  }

  quadric_t& operator+=(const quadric_t& q2) {
  // prevent cancelling out when quadrics point in opposite direction
  real coeff = (dot(vec3r{hess}, vec3r{q2.hess}) < 0) ? -1.0 : 1.0;
  // coeff = 1;
  for(size_t i = 0; i < values.size(); ++i) {
    values[i] += coeff * q2.values[i]; 
  }
  hess += coeff * q2.hess;
  return *this;
}

  std::array<real, 10> values;
  vec4r hess;
};

quadric_t operator+(const quadric_t& q1, const quadric_t& q2) {
  quadric_t q;
  // prevent cancelling out when quadrics point in opposite direction
  real coeff = (dot(vec3r{q1.hess}, vec3r{q2.hess}) < 0) ? -1.0 : 1.0;
  // coeff = 1;
  for(size_t i = 0; i < q.values.size(); ++i) {
    q.values[i] = q1.values[i] + coeff * q2.values[i]; 
  }
  q.hess = q1.hess + coeff * q2.hess;
  return q;
}

struct edge_t
{
  edge_t(surfel_id_t p1, surfel_id_t p2) 
   :a{p1 < p2 ? p1 : p2}
   ,b{p1 < p2 ? p2 : p1}
   {};

  surfel_id_t a;
  surfel_id_t b;
};

bool operator==(const edge_t& e1, const edge_t& e2) {
  if(e1.a == e2.a)   {
    return e1.b == e2.b;
  }
  return false;
}

bool operator<(const edge_t& e1, const edge_t& e2) {
  if(e1.a < e2.a) {
    return true;
  }
  else {
    if(e1.a == e2.a) {
      return e1.b < e2.b;
    }
    else return false;
  }
}

std::ostream& operator<<(std::ostream& os, const edge_t& e) {
  os << "<" << e.a << "--" << e.b << ">";
  return os;
}

struct contraction_op;

struct contraction {
  contraction(edge_t e, quadric_t quad, real err, surfel surf)
   :edge{e}
   ,quadric{quad}
   ,error{err}
   ,new_surfel{surf}
  {}

  edge_t edge;
  quadric_t quadric;
  real error;
  surfel new_surfel;

  contraction_op* cont_op;
};

bool operator<(const contraction& c1, const contraction& c2) {
  return c1.error < c2.error;
}

struct contraction_op {
  contraction_op(contraction* c)
   :cont{c}
  {};

  contraction* cont;
};

bool operator<(const contraction_op& c1, const contraction_op& c2) {
  // invalid op -> must return false to form strict-weak ordering
  // otherwise undefined behaviour for sort
  return c1.cont->error > c2.cont->error;
}


quadric_t edge_quadric(const vec3f& normal_p1, const vec3f& normal_p2, const vec3r& p1, const vec3r& p2);

}
}

const size_t num_surfels_per_node = 3000;
const size_t num_nodes_per_level = 14348907;
namespace std {
template <> struct hash<lamure::pre::edge_t>
{
  size_t operator()(lamure::pre::edge_t const & e) const noexcept
  {
    uint16_t h1 = e.a.node_idx * num_surfels_per_node + e.a.surfel_idx;  
    uint16_t h2 = e.b.node_idx * num_surfels_per_node + e.b.surfel_idx;  
    size_t hash = h1;
    hash += h2 >> 16;
    return hash;
  }
};
template <> struct hash<lamure::surfel_id_t>
{
  size_t operator()(lamure::surfel_id_t const & s) const noexcept
  {
    return s.node_idx * num_surfels_per_node + s.surfel_idx;  
  }
};
}
namespace lamure {
namespace pre {

bool a = false;

real sum(const mat4r& quadric) {  
  real sum = 0;
  for(int i = 0; i < 16; ++i) {
    sum += quadric[i] * quadric[i];
  }
  return sum;
}

vec3b n_to_c(vec3f normal) {
  // normal = normal * 255;
  normal = (normal * 0.5f + 0.5f) * 255;
  return vec3b{uint8_t(normal.x), uint8_t(normal.y), uint8_t(normal.z)}; 
}
vec3b n_to_cb(vec3f normal) {
  normal.x = (normal.x < 0) ? 0 : 1;
  normal.y = (normal.y < 0) ? 0 : 1;
  normal.z = (normal.z < 0) ? 0 : 1;
  normal = normal * 255;
  // normal = (normal * 0.5f + 0.5f) * 255;
  return vec3b{uint8_t(normal.x), uint8_t(normal.y), uint8_t(normal.z)}; 
}

static vec3b heatmap(float norm_val) {
  vec3f color{0.0f};
  float third = 1 / 3.0f;
  if(norm_val <= third) {
    color.g = 1.0f;
    color.r =std::max(0.0f, (third - norm_val) * 3.0f);
  }
  else if(norm_val <= third * 2.0f) { 
    norm_val -= third;
    color.b =std::max(0.0f, norm_val * 3.0f);
    color.g =std::max(0.0f, (third - norm_val) * 3.0f);
  }
  else {
    norm_val -= 2.0f * third;
    color.r =std::max(0.0f, norm_val * 3.0f);
    color.b =std::max(0.0f, (third - norm_val) * 3.0f);
  }
  color *= 255;
  return vec3b{uint8_t(color.x), uint8_t(color.y), uint8_t(color.z)};
}

real qlength(const mat4r& quadric) {
  real a = sqrt(quadric[0]);
  vec4r n = vec4r{quadric[0], quadric[1], quadric[2], quadric[3]};
  n /= a;
  return scm::math::length_sqr(n);
}


surfel_mem_array reduction_pair_contraction::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node,
          const bvh& tree,
          const size_t start_node_id) const
{
  const uint32_t fan_factor = input.size();
  size_t num_surfels = 0;
  size_t min_num_surfels = input[0]->length(); 
  //compute max total number of surfels from all nodes
  for (size_t node_idx = 0; node_idx < fan_factor; ++node_idx) {
    num_surfels += input[node_idx]->length();

    if (input[node_idx]->length() < min_num_surfels) {
      min_num_surfels = input[node_idx]->length();
    }
  }

  std::map<surfel_id_t, quadric_t> quadrics{};
  std::vector<std::vector<surfel>> node_surfels{input.size() + 1, std::vector<surfel>{}};
  std::set<edge_t> edges{};
  // accumulate edges and point quadrics
  for (node_id_type node_idx = 0; node_idx < fan_factor; ++node_idx) {
    for (size_t surfel_idx = 0; surfel_idx < input[node_idx]->length(); ++surfel_idx) {
      
      surfel curr_surfel = input[node_idx]->read_surfel(surfel_idx);
      // save surfel
      node_surfels[node_idx].push_back(curr_surfel);
      surfel_id_t curr_id = surfel_id_t{node_idx, surfel_idx};

      assert(node_idx < num_nodes_per_level && surfel_idx < num_surfels_per_node);
      // get and store neighbours
      auto nearest_neighbours = get_local_nearest_neighbours(input, number_of_neighbours_, curr_id);
      std::vector<surfel_id_t> neighbour_ids{};
      for (const auto& pair : nearest_neighbours) {
        neighbour_ids.push_back(pair.first);
      }

      quadric_t curr_quadric{};
      for (auto const& neighbour : nearest_neighbours) {
        edge_t curr_edge = edge_t{curr_id, neighbour.first}; 
        assert(neighbour.first.node_idx < num_nodes_per_level && neighbour.first.surfel_idx <num_surfels_per_node);
        if (edges.find(curr_edge) == edges.end()) {
          edges.insert(curr_edge);
        }
          // accumulate quadric
          surfel neighbour_surfel = input[neighbour.first.node_idx]->read_surfel(neighbour.first.surfel_idx);
          curr_quadric += edge_quadric(curr_surfel.normal(), neighbour_surfel.normal(), curr_surfel.pos(), neighbour_surfel.pos());
      }
      quadrics[curr_id] = curr_quadric;
    }
  }


  // allocate space for new surfels that will be created
  node_surfels.back() = std::vector<surfel>{num_surfels - surfels_per_node};
  #ifdef DEBUG
  real error_min = std::numeric_limits<real>::max();
  real error_max = 0;
  std::cout << "creating contractions" << std::endl;
  auto create_contraction = [&node_surfels, &quadrics, &error_max, &error_min](const edge_t& edge)->contraction
  #else
  auto create_contraction = [&node_surfels, &quadrics](const edge_t& edge)->contraction
  #endif
   {
    const surfel& surfel1 = node_surfels[edge.a.node_idx][edge.a.surfel_idx];
    const surfel& surfel2 = node_surfels[edge.b.node_idx][edge.b.surfel_idx];
    // new surfel is mean of both old surfels
    surfel new_surfel = surfel{(surfel1.pos() + surfel2.pos()) * 0.5f,
                          (surfel1.color() + surfel2.color()) * 0.5f,
                          (surfel1.radius() + surfel2.radius()) * 0.5f,
                          (normalize(surfel1.normal() + surfel2.normal()))
                          };
    auto new_quadric = (quadrics.at(edge.a) + quadrics.at(edge.b));
    real error = new_quadric.error(new_surfel.pos());
    real error1 = new_quadric.error(surfel1.pos());
    real error2 = new_quadric.error(surfel2.pos());

    if(error1 < error) {
      if(error2 < error1) {
        new_surfel = surfel2;
        error = error2;
      }
      else {
        new_surfel = surfel1;
        error = error1;
      }
    }
    else if(error2 < error) {
      new_surfel = surfel2;
      error = error2;
    }
    #ifdef DEBUG
    if (error > error_max) error_max = error;
    if (error < error_min) error_min = error;
    #endif
    return contraction{edge, std::move(new_quadric), error, std::move(new_surfel)};
  };

  // store contractions and operations in queue
  std::unordered_map<surfel_id_t, std::map<surfel_id_t,std::shared_ptr<contraction>>> contractions{};
  std::vector<std::shared_ptr<contraction_op>> contraction_queue{};
  for (const auto& edge : edges) {
    if (contractions[edge.a].find(edge.b) != contractions[edge.a].end()) {
      continue;
    }
    // map contraction to both surfels
    contractions[edge.a][edge.b] = std::make_shared<contraction>(create_contraction(edge));
    contractions[edge.b][edge.a] = contractions.at(edge.a).at(edge.b);
    // check surfel id bounds
    assert(edge.a.node_idx < num_nodes_per_level && edge.a.surfel_idx <num_surfels_per_node);
    assert(edge.b.node_idx < num_nodes_per_level && edge.b.surfel_idx <num_surfels_per_node);
    // store contraction operation pointing to new contraction
    contraction_op op{contractions.at(edge.a).at(edge.b).get()};
    contraction_queue.push_back(std::make_shared<contraction_op>(op));  
    // let contraction point to related contraction_op
    contractions.at(edge.a).at(edge.b)->cont_op = contraction_queue.back().get();  
    //check pointer correctness
    assert(contraction_queue.back().get() == contraction_queue.back()->cont->cont_op);
    assert(contractions.at(edge.a).at(edge.b).get() == contractions.at(edge.a).at(edge.b).get()->cont_op->cont);
    assert(contractions.at(edge.a).at(edge.b).get() == contractions.at(edge.b).at(edge.a).get());
  }
  #ifdef DEBUG
  std::cout << "error min " << error_min << " max " << error_max << std::endl;

  error_min = std::numeric_limits<real>::max();
  error_max = 0;
  for (node_id_type node_idx = 0; node_idx < fan_factor; ++node_idx) {
    for (size_t surfel_idx = 0; surfel_idx < input[node_idx]->length(); ++surfel_idx) {
      real error = 0;
      size_t i = 0;
      for(const auto& pair : contractions.at(surfel_id_t{node_idx, surfel_idx})) {
        error += pair.second->error;
        ++i;
      }
      error /= real(i);
      if (error > error_max) error_max = error;
      if (error < error_min) error_min = error;
    }
  }
  for (node_id_type node_idx = 0; node_idx < fan_factor; ++node_idx) {
    for (size_t surfel_idx = 0; surfel_idx < input[node_idx]->length(); ++surfel_idx) {
      surfel& curr_surfel = node_surfels[node_idx][surfel_idx];
      real error = 0;
      size_t i = 0;
      for(const auto& pair : contractions.at(surfel_id_t{node_idx, surfel_idx})) {
        error += pair.second->error;
        ++i;
      }
      error /= real(i);
      #ifdef ERROR_COLOR
      // error of contraction
      curr_surfel.color() = heatmap((error - error_min) / (error_max - error_min));
      // binary dir of surfel normal
      // curr_surfel.color() = n_to_c(curr_surfel.normal());
      // write to orig data
      input[node_idx]->write_surfel(curr_surfel, surfel_idx);
      curr_surfel.color() = vec3b{127, 127, 127};
      #endif
    }
  }
  size_t n_min = number_of_neighbours_;
  size_t n_max = 0;
  std::cout << "doing contractions" << std::endl;
  #endif
  // work off queue until target num of surfels is reached
  for (size_t i = 0; i < num_surfels - surfels_per_node; ++i) {
    // update queue, cheapest contraction at the back
    std::sort(contraction_queue.begin(), contraction_queue.end(), 
      [](const std::shared_ptr<contraction_op>& a, const std::shared_ptr<contraction_op>& b){
         return *a < *b;
       }
      );

    contraction curr_contraction = *(contraction_queue.back()->cont);
    contraction_queue.pop_back();
    for (int i = contraction_queue.size() - 1; i > 0; --i) {
      if (curr_contraction.error > contraction_queue[i]->cont-> error) {
        throw std::exception();
      }
    }
    
    surfel_id_t new_id = surfel_id_t{node_surfels.size()-1, i};
    // save new surfels in vector at back
    surfel& new_surfel = curr_contraction.new_surfel;
    // save new surfel
    #ifdef ERROR_COLOR
    new_surfel.color() = heatmap((curr_contraction.error - error_min) / (error_max - error_min));
    #endif
    node_surfels.back()[i] = new_surfel;

    const surfel_id_t& old_id_1 = curr_contraction.edge.a;
    const surfel_id_t& old_id_2 = curr_contraction.edge.b;
    // invalidate old surfels
    #ifdef LEAF_REMOVAL
    if (old_id_1.node_idx < fan_factor) {
      surfel surf = node_surfels[old_id_1.node_idx][old_id_1.surfel_idx];
      surf.color() = vec3b{255,255,255};
      input[old_id_1.node_idx]->write_surfel(surf, old_id_1.surfel_idx);
    }
    #endif
    node_surfels[old_id_1.node_idx][old_id_1.surfel_idx].radius() = -1.0f;
    #ifdef LEAF_REMOVAL
    if (old_id_2.node_idx < fan_factor) {
      surfel surf = node_surfels[old_id_2.node_idx][old_id_2.surfel_idx];
      surf.color() = vec3b{255,255,255};
      input[old_id_2.node_idx]->write_surfel(surf, old_id_2.surfel_idx);
    }
    #endif
    node_surfels[old_id_2.node_idx][old_id_2.surfel_idx].radius() = -1.0f;
    // add new point quadric
    quadrics[new_id] = curr_contraction.quadric;
    // delete old point quadrics
    quadrics.erase(curr_contraction.edge.a);
    quadrics.erase(curr_contraction.edge.b);
   
    auto update_contraction = [&create_contraction, &contractions, &curr_contraction]
      (const surfel_id_t& new_id, const surfel_id_t& old_id, const std::pair<const surfel_id_t, std::shared_ptr<contraction>>& cont) {
        edge_t new_edge = edge_t{new_id, cont.first};
        // store new contraction
        contractions[new_id][cont.first] = std::make_shared<contraction>(create_contraction(new_edge));
        // update contractions of neighbour
        assert(contractions.find(cont.first) != contractions.end());
        contractions.at(cont.first).erase(old_id);
        contractions.at(cont.first)[new_id] = contractions.at(new_id).at(cont.first);
        // get attached operation
        contraction_op* operation = cont.second->cont_op;
        // transfer contraction op
        contractions.at(new_id).at(cont.first)->cont_op = operation;
        operation->cont = contractions.at(new_id).at(cont.first).get();
        // check for pointer correctness
        assert(contractions.at(new_edge.a).at(new_edge.b).get() == contractions.at(new_edge.a).at(new_edge.b).get()->cont_op->cont);
        assert(contractions.at(new_edge.a).at(new_edge.b).get() == contractions.at(new_edge.b).at(new_edge.a).get());
    };

    size_t neighbours = 0;
    for(const auto& cont : contractions.at(old_id_1)) {
      if (cont.first != old_id_2) {
        #ifdef LIMIT_NEIGHBOURS
        if(neighbours >= number_of_neighbours_) {
          // already added -> remove duplicate contractions
          contractions.at(cont.first).erase(old_id_1);
          // // and invalidate respective operation
          cont.second->cont_op->cont = nullptr;
        }
        else
        #endif
        {
          update_contraction(new_id, old_id_1, cont);
          assert(contractions.at(cont.first).find(old_id_1) == contractions.at(cont.first).end());
          ++neighbours;
        }
      }
      else {
        // invalidate operation
        cont.second->cont_op->cont = nullptr;
      }
    }
    // in case no new contractions were created yet
    if(contractions.find(new_id) == contractions.end()) {
      // std::cout << old_id_1 << std::endl;
      contractions[new_id];
    }
    for(const auto& cont : contractions.at(old_id_2)) {
      if (cont.first != old_id_1) {
        #ifdef LIMIT_NEIGHBOURS
        if(contractions.at(new_id).find(cont.first) == contractions.at(new_id).end() && neighbours < number_of_neighbours_)
        #else
        if(contractions.at(new_id).find(cont.first) == contractions.at(new_id).end()) 
        #endif
        {
          update_contraction(new_id, old_id_2, cont);
          ++neighbours;
        }
        else {
          // already added -> remove duplicate contractions
          contractions.at(cont.first).erase(old_id_2);
          // // and invalidate respective operation
          cont.second->cont_op->cont = nullptr;
        }
        assert(contractions.at(cont.first).find(old_id_2) == contractions.at(cont.first).end());
      }
      else {
        // invalidate operation
        cont.second->cont_op->cont = nullptr;
      }
    }
    #ifdef DEBUG
    if(neighbours < n_min) {
      n_min = neighbours;
    }    
    if(neighbours > n_max) {
      n_max = neighbours;
    }
    #endif
    // remove old mapping
    contractions.erase(old_id_1);
    contractions.erase(old_id_2);
    //remove invalid contraction operations
    contraction_queue.erase(std::remove_if(contraction_queue.begin(), contraction_queue.end(), [](const std::shared_ptr<contraction_op>& op){return op->cont == nullptr;}), contraction_queue.end());
  }
  #ifdef DEBUG
  std::cout << "neighbours min " << n_min << " max " << n_max << std::endl;
  std::cout << "copying surfels" << std::endl;
  #endif
  surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);
  for (auto& node : node_surfels) {
    for (auto& surfel : node) {
      if (surfel.radius() > 0.0f) {
        mem_array.mem_data()->push_back(surfel);
      }
    }
  }
  mem_array.set_length(mem_array.mem_data()->size());

  reduction_error = 0.0;

  return mem_array;
}


lamure::pre::quadric_t edge_quadric(const vec3f& normal_p1, const vec3f& normal_p2, const vec3r& p1, const vec3r& p2)
{
  vec3r edge_dir = p2 - p1;
  vec3r tangent = cross(vec3r(normal_p1 + (dot(normal_p1, normal_p2) < 0.0 ? -1.0 : 1.0f) * normal_p2), edge_dir);

  vec3r normal = cross(tangent, edge_dir);
  // dont take root for more smooth result
  normal /= normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;

  vec4r hessian = vec4r{normal, -dot(p1, normal)}; 
  return quadric_t{hessian};
}

lamure::real quadric_error(const vec3r& p, const quadric_t& quadric)
{
  return quadric.error(p);
}

// get k nearest neighbours in simplification nodes
std::vector<std::pair<surfel_id_t, real>> 
get_local_nearest_neighbours(const std::vector<surfel_mem_array*>& input,
                             size_t num_local_neighbours,
                             surfel_id_t const& target_surfel) {

    //size_t current_node = target_surfel.node_idx;
    vec3r center = input[target_surfel.node_idx]->read_surfel(target_surfel.surfel_idx).pos();

    std::vector<std::pair<surfel_id_t, real>> candidates;
    real max_candidate_distance = std::numeric_limits<real>::infinity();

    for (size_t local_node_id = 0; local_node_id < input.size(); ++local_node_id) {
        for (size_t surfel_id = 0; surfel_id < input[local_node_id]->length(); ++surfel_id) {
            if (surfel_id != target_surfel.surfel_idx || local_node_id != target_surfel.node_idx) {
                const surfel& current_surfel = input[local_node_id]->read_surfel(surfel_id);
                real distance_to_center = scm::math::length_sqr(center - current_surfel.pos());

                if (candidates.size() < num_local_neighbours || (distance_to_center < max_candidate_distance)) {
                    if (candidates.size() == num_local_neighbours)
                        candidates.pop_back();

                    candidates.push_back(std::make_pair(surfel_id_t(local_node_id, surfel_id), distance_to_center));

                    for (uint16_t k = candidates.size() - 1; k > 0; --k) {
                        if (candidates[k].second < candidates[k - 1].second) {
                            std::swap(candidates[k], candidates[k - 1]);
                        }
                        else
                            break;
                    }

                    max_candidate_distance = candidates.back().second;
                }
            }
        }
    }

    return candidates;
}

} // namespace pre
} // namespace lamure
