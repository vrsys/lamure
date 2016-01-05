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
#include <map>
#include <unordered_map>

namespace lamure {

bool operator==(const surfel_id_t& a, const surfel_id_t& b) {
  return a.node_idx == b.node_idx && a.surfel_idx == b.surfel_idx;
}

bool operator<(const surfel_id_t& a, const surfel_id_t& b) {
  if(a.node_idx < b.node_idx) {
    return true;
  }
  else {
    if(a.node_idx == b.node_idx) {
      return a.surfel_idx < b.surfel_idx;
    }
    else return false;
  }
}

namespace pre {
struct edge_t
{
  edge_t(surfel_id_t p1, surfel_id_t p2) 
   :a{p1.surfel_idx < p2.surfel_idx ? p1 : p2}
   ,b{p1.surfel_idx >= p2.surfel_idx ? p1 : p2}
   {};

   // operator<(const edge_t& e) const {
   //  return 
   // }


  surfel_id_t a;
  surfel_id_t b;
};

bool operator==(const edge_t& e1, const edge_t& e2) {
  if(e1.a == e2.a)   {
    return e1.b == e2.b;
  }
  return false;
}

struct contraction_op;

struct contraction {
  contraction(edge_t e, mat4r quad, real err, surfel surf)
   :edge{e}
   ,quadric{quad}
   ,error{err}
   ,new_surfel{surf}
  {}

  edge_t edge;
  mat4r quadric;
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
  // return c1.cont->error < c2.cont->error;
  return c1.cont->error > c2.cont->error;
}

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
}
namespace lamure {
namespace pre {


size_t num_neighbours = 20;

surfel_mem_array reduction_pair_contraction::
create_lod(bvh* tree,
          real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node) const
{
  std::list<surfel> output_surfels{};

  const uint32_t fan_factor = input.size();
  size_t num_points = 0;
  size_t min_num_surfels = input[0]->length(); 
  //compute max total number of surfels from all nodes
  for (size_t node_idx = 0; node_idx < fan_factor; ++node_idx) {
    num_points += input[node_idx]->length();

    if (input[node_idx]->length() < min_num_surfels) {
      min_num_surfels = input[node_idx]->length();
    }
  }

  std::map<surfel_id_t, mat4r> quadrics{};
  std::map<surfel_id_t, std::vector<surfel_id_t>> neighbours{};
  std::vector<std::vector<surfel>> surfels{input.size() + 1, std::vector<surfel>{}};
  std::unordered_set<edge_t> edges{};

  // accumulate edges and point quadrics
  size_t offset = 0;
  for (node_id_type node_idx = 0; node_idx < fan_factor; ++node_idx) {
    for (size_t surfel_idx = 0; surfel_idx < input[node_idx]->length(); ++surfel_idx) {
      
      surfel curr_surfel = input[node_idx]->read_surfel(surfel_idx);
      // save surfel
      surfels[node_idx].push_back(curr_surfel);

      surfel_id_t curr_id = surfel_id_t{node_idx, surfel_idx};
      assert(node_idx < num_nodes_per_level && surfel_idx < num_surfels_per_node);
      // get and store neighbours
      auto nearest_neighbours = get_local_nearest_neighbours(input, num_neighbours, curr_id);
      std::vector<surfel_id_t> neighbour_ids{};
      for(size_t i = 0; i < nearest_neighbours.size(); ++i) {
        const auto& pair = nearest_neighbours[i];
        neighbour_ids.push_back(pair.first);
      }
      neighbours[curr_id] = neighbour_ids;

      mat4r curr_quadric = mat4r::zero();
      for (auto const& neighbour : nearest_neighbours) {
        // store edge
        edge_t curr_edge = edge_t(curr_id, neighbour.first); 
        if (edges.find(curr_edge) == edges.end()) {
          assert(neighbour.first.node_idx < num_nodes_per_level && neighbour.first.surfel_idx <num_surfels_per_node);
          edges.insert(curr_edge);
        }
        // accumulate quadric
        surfel neighbour_surfel = input[neighbour.first.node_idx]->read_surfel(neighbour.first.surfel_idx);
        curr_quadric += edge_quadric(curr_surfel.normal(), curr_surfel.pos(), neighbour_surfel.pos());
      }
      quadrics[curr_id] = curr_quadric;
    }
    offset += input[node_idx]->length();
  }

  auto create_contraction = [&surfels, &quadrics](const edge_t& edge)->contraction {
    surfel surfel1 = surfels[edge.a.node_idx][edge.a.surfel_idx];
    surfel surfel2 = surfels[edge.b.node_idx][edge.b.surfel_idx];
    // new surfel is mean of both old surfels
    surfel new_surfel = surfel{(surfel1.pos() + surfel2.pos()) * 0.5f,
                          (surfel1.color() + surfel2.color()) * 0.5f,
                          (surfel1.radius() + surfel2.radius()) * 0.5f,
                          (surfel1.normal() + surfel2.normal()) * 0.5f
                          };
    mat4r new_quadric = quadrics.at(edge.a) + quadrics.at(edge.b);
    real error = quadric_error(new_surfel.pos(), new_quadric);
    return contraction{edge, new_quadric, error, new_surfel};
  };

  // store contractions and operations in queue
  std::unordered_map<edge_t, std::shared_ptr<contraction>> contractions{};
  std::vector<std::shared_ptr<contraction_op>> contraction_queue{};
  for(const auto& edge : edges) {
    contractions[edge] = std::make_shared<contraction>(create_contraction(edge));
    
    contraction_op op{std::addressof(*contractions.at(edge))};
    contraction_queue.push_back(std::make_shared<contraction_op>(op));  

    contractions.at(edge)->cont_op = std::addressof(*contraction_queue.back());  
  }

  // work off queue until target num of surfels is reached
  size_t new_num_points = num_points;
  while(new_num_points > min_num_surfels) {
    // get next contraction
    contraction curr_contraction = *(contraction_queue.back()->cont);
    const edge_t& curr_edge = curr_contraction.edge;
    // save new surfel
    surfel_id_t new_id = surfel_id_t{fan_factor + 1, surfels[fan_factor + 1].size()};
    const surfel& new_surfel = curr_contraction.new_surfel;
    surfels[new_id.node_idx].push_back(new_surfel);
    
    // delete old point quadrics
    quadrics.erase(curr_contraction.edge.a);
    quadrics.erase(curr_contraction.edge.b);
    // add new point quadric
    quadrics[new_id] = curr_contraction.quadric;
    
    // invalidate old surfels
    surfels[curr_contraction.edge.a.node_idx][curr_contraction.edge.a.surfel_idx].radius() = 0.0f;
    surfels[curr_contraction.edge.b.node_idx][curr_contraction.edge.b.surfel_idx].radius() = 0.0f;

    auto update_contraction = [&contractions, &contraction_queue](const edge_t& old_edge, const edge_t& new_edge){
      // get contraction
      contraction cont = *(contractions.at(old_edge));
      cont.edge = new_edge;
    // remove edge quadric from them
    // add new edge quadric
      // remove old contraction
      contractions.erase(old_edge);
      // insert new one
      contractions[new_edge] = std::make_shared<contraction>(cont);
      // update attached operation
      contraction_op& operation = *cont.cont_op;
      operation.cont = std::addressof(*(contractions.at(new_edge)));
    };

    for(const auto& neighbour : neighbours[curr_contraction.edge.a]) {
      update_contraction(edge_t{curr_contraction.edge.a, neighbour}, curr_edge);    
    }
    for(const auto& neighbour : neighbours[curr_contraction.edge.b]) {
      update_contraction(edge_t{curr_contraction.edge.b, neighbour}, curr_edge);    
    }
    // update queue, cheapest contraction at the back
    std::sort(contraction_queue.begin(), contraction_queue.end());
  }

  // save valid surfels in mem array
  surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);
  for (const auto& node : surfels) {
    for (const auto& surfel : node) {
      if (surfel.radius() > 0.0f) {
        mem_array.mem_data()->push_back(surfel);
      }
    }
  }
  mem_array.set_length(mem_array.mem_data()->size());

  reduction_error = 0.0;

  return mem_array;
}


lamure::mat4r edge_quadric(const vec3f& normal_p1, const vec3r& p1, const vec3r& p2)
{
    vec3r edge_dir = normalize(p2 - p1);
    vec3r tangent = normalize(cross(vec3r(normal_p1), edge_dir));

    vec3r normal = normalize(cross(tangent, edge_dir));
    vec4r hessian = vec4r{normal, dot(p1, normal)}; 

    lamure::mat4r quadric = mat4r{hessian * hessian.x,
                                  hessian * hessian.y,
                                  hessian * hessian.z,
                                  hessian * hessian.w};

    return quadric;
}

lamure::real quadric_error(const vec3r& p, const mat4r& quadric)
{
  vec4r p_transformed = quadric * p;
  return dot(p, vec3r{p_transformed} / p_transformed.w);
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
            if (surfel_id != target_surfel.surfel_idx) {
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
