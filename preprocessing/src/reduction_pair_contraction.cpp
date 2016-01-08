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

namespace pre {
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

  std::shared_ptr<contraction_op> cont_op;
};

bool operator<(const contraction& c1, const contraction& c2) {
  return c1.error < c2.error;
}

struct contraction_op {
  contraction_op(std::shared_ptr<contraction> c)
   :cont{c}
  {};

  std::shared_ptr<contraction> cont;
};

bool operator<(const contraction_op& c1, const contraction_op& c2) {
  // invalid op -> sort to front
  if (c1.cont == nullptr) {
    return false;
  }
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

  const surfel_id_t F_SURFEL = surfel_id_t{1,1321};
  const edge_t F_EDGE = edge_t{surfel_id_t{1,1321}, surfel_id_t{1,1325}};

  std::map<surfel_id_t, mat4r> quadrics{};
  std::map<surfel_id_t, std::vector<surfel_id_t>> neighbours{};
  std::vector<std::vector<surfel>> node_surfels{input.size() + 1, std::vector<surfel>{}};
  std::set<edge_t> edges{};
  size_t i = 0;
  std::cout << "creating quadrics" << std::endl;
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
      neighbours[curr_id] = neighbour_ids;

      mat4r curr_quadric = mat4r::zero();
      for (auto const& neighbour : nearest_neighbours) {
          edge_t curr_edge = edge_t{curr_id, neighbour.first}; 
        if(curr_id == F_SURFEL || neighbour.first == F_SURFEL ) {
          std::cout << curr_edge << std::endl;
        }
        if (curr_edge == F_EDGE) {
            std::cout << "found --------------------------------------------------------------------------------------------------------------------------------------" << curr_edge << std::endl;
          }
          assert(neighbour.first.node_idx < num_nodes_per_level && neighbour.first.surfel_idx <num_surfels_per_node);
        if (edges.find(curr_edge) == edges.end()) {
        // if (std::find(edges.begin(), edges.end(), curr_edge) == edges.end()) {
          // edges.push_back(curr_edge);
          edges.insert(curr_edge);
          ++i;
                  if (curr_edge == F_EDGE) {
            std::cout << "added --------------------------------------------------------------------------------------------------------------------------------------" << curr_edge << std::endl;
          }
        }
        else {

        }
        // accumulate quadric
        surfel neighbour_surfel = input[neighbour.first.node_idx]->read_surfel(neighbour.first.surfel_idx);
        curr_quadric += edge_quadric(curr_surfel.normal(), curr_surfel.pos(), neighbour_surfel.pos());
      }
      quadrics[curr_id] = curr_quadric;
    }
  }
  std::cout << "iu" << i << std::endl;
  // allocate space for new surfels that will be created
  node_surfels.back() = std::vector<surfel>{num_surfels - surfels_per_node};

  if(edges.find(F_EDGE) == edges.end()) {
    std::cout << "ffffffffffffff" << std::endl;
    throw std::runtime_error("fffffffff");
  }

  std::cout << "creating contractions" << std::endl;
  auto create_contraction = [&node_surfels, &quadrics](const edge_t& edge)->contraction {
    surfel surfel1 = node_surfels[edge.a.node_idx][edge.a.surfel_idx];
    surfel surfel2 = node_surfels[edge.b.node_idx][edge.b.surfel_idx];
    // new surfel is mean of both old surfels
    surfel new_surfel = surfel{(surfel1.pos() + surfel2.pos()) * 0.5f,
                          (surfel1.color() + surfel2.color()) * 0.5f,
                          (surfel1.radius() + surfel2.radius()) * 0.5f,
                          (surfel1.normal() + surfel2.normal()) * 0.5f
                          };
      // new_quadric = quadrics.at(edge.a) + quadrics.at(edge.b);
    mat4r new_quadric, a, b;
    try {
      a = quadrics.at(edge.a);
    }
    catch(std::exception& e) {
      std::cout << "cant find " << edge.a << std::endl;
      throw std::runtime_error("cant findor");
    }
    try {
      b = quadrics.at(edge.b);
    }
    catch(std::exception& e) {
      std::cout << "cant find " << edge.b << std::endl;
      throw std::runtime_error("cant findor");
    }
    new_quadric = a + b;
    real error = quadric_error(new_surfel.pos(), new_quadric);
    return contraction{edge, new_quadric, error, new_surfel};
  };

  // store contractions and operations in queue
  std::unordered_map<edge_t, std::shared_ptr<contraction>> contractions{};
  std::vector<std::shared_ptr<contraction_op>> contraction_queue{};
  i = 0;
  for (const auto& edge : edges) {
    // std::cout << edge << std::endl;
    // if(edge.a == F_SURFEL || edge.b == F_SURFEL) {
    //   std::cout << "contraction for " << edge << " found "<< std::endl;
    // }
    contractions[edge] = std::make_shared<contraction>(create_contraction(edge));
    assert(edge.a.node_idx < num_nodes_per_level && edge.a.surfel_idx <num_surfels_per_node);
    assert(edge.b.node_idx < num_nodes_per_level && edge.b.surfel_idx <num_surfels_per_node);
    // store contraction operation pointing to new contraction
    contraction_op op{contractions.at(edge)};
    contraction_queue.push_back(std::make_shared<contraction_op>(op));  
    // let contraction point to related contraction_op
    contractions.at(edge)->cont_op = contraction_queue.back();  
    // for now check pointer correctness
    assert(contraction_queue.back().get() == contraction_queue.back()->cont->cont_op.get());
    assert(contractions[edge].get() == contractions[edge].get()->cont_op->cont.get());
    // if(edge == F_EDGE) {
    //   std::cout << "created contraction ----------------------" << F_EDGE << std::endl;
    // }
    ++i;
  }
  std::cout << "contractions " << i << std::endl; 

  std::cout << "doing contractions" << std::endl;
  // work off queue until target num of surfels is reached
  for (size_t i = 0; i < num_surfels - surfels_per_node; ++i) {
    // get next contraction
    if(contraction_queue.back()->cont == nullptr) {
      contraction_queue.pop_back();
      --i;
      continue;
    }
    contraction curr_contraction = *(contraction_queue.back()->cont);
    contraction_queue.pop_back();
    surfel_id_t new_id = surfel_id_t{node_surfels.size()-1, i};
    std::cout << " contraction of " << curr_contraction.edge << " to " << new_id << "--------------------------------------------------" << std::endl;
    // save new surfels in vector at back
    const surfel& new_surfel = curr_contraction.new_surfel;
    // save new surfel
    node_surfels.back()[i] = new_surfel;
    // invalidate old surfels
    const surfel_id_t& old_id_1 = curr_contraction.edge.a;
    const surfel_id_t& old_id_2 = curr_contraction.edge.b;
    node_surfels[old_id_1.node_idx][old_id_1.surfel_idx].radius() = -1.0f;
    node_surfels[old_id_2.node_idx][old_id_2.surfel_idx].radius() = -1.0f;

    // delete old point quadrics
    // quadrics.erase(curr_contraction.edge.a);
    // quadrics.erase(curr_contraction.edge.b);
    // add new point quadric
    quadrics[new_id] = curr_contraction.quadric;

    auto update_contraction = [&edges, &contractions, &contraction_queue, &node_surfels, &create_contraction]
      (const surfel_id_t& new_s, const surfel_id_t& old_s, const surfel_id_t& neigh_s) {
      edge_t old_edge = edge_t{old_s, neigh_s};
      edge_t new_edge = edge_t{new_s, neigh_s};
      // get contraction
      contraction cont{old_edge, mat4r{}, 0, surfel{}};
      try {
        cont = *(contractions.at(old_edge));
      }
      catch (std::exception& e) {
        std::cout << "missing contraction " << old_edge << std::endl;
        throw std::runtime_error("missing contraction");
      }
      // create and insert new contraction
      contractions[new_edge] = std::make_shared<contraction>(create_contraction(new_edge));
      // transfer contraction op
      contractions.at(new_edge)->cont_op = contractions.at(old_edge)->cont_op;
      // update attached operation
      std::shared_ptr<contraction_op> operation = contractions.at(old_edge)->cont_op;

      try {
        // std::cout << "setting to op from" << operation->cont->edge << " to "<< new_edge << std::endl;
        operation->cont = contractions.at(new_edge);
      }
      catch (std::exception& e) {
        std::cout << "missing edge " << new_edge << std::endl;
        throw std::runtime_error("missing edge");
      }
      // remove old contraction
      // std::cout << "erasing" << old_edge << std::endl;
      contractions.erase(old_edge);
    };

    std::vector<surfel_id_t> new_neighbours{};
    for (const auto& neighbour : neighbours[old_id_1]) {
      if(neighbour != old_id_2) {
        std::cout<< "new " << edge_t{new_id, neighbour} << " old " << edge_t{old_id_1, neighbour} << std::endl;
        update_contraction(new_id, old_id_1, neighbour);
        new_neighbours.push_back(neighbour);  
        *(std::find(neighbours[neighbour].begin(), neighbours[neighbour].end(), old_id_1)) = new_id;  
      }
    }
    for (const auto& neighbour : neighbours[old_id_2]) {
      if(neighbour != old_id_1) {
        if(std::find(new_neighbours.begin(), new_neighbours.end(), neighbour) == new_neighbours.end()) {
          std::cout<< "new " << edge_t{new_id, neighbour} << " old " << edge_t{old_id_2, neighbour} << std::endl;
          update_contraction(new_id, old_id_2, neighbour);    
          new_neighbours.push_back(neighbour);    
          *(std::find(neighbours[neighbour].begin(), neighbours[neighbour].end(), old_id_2)) = new_id;  
        }
        // neighbour is also neighbour of old_id_1
        else {
          edge_t old_edge = edge_t{old_id_2, neighbour};
          std::cout<< "skipping new " << edge_t{new_id, neighbour} << " old " << old_edge << std::endl;
          //invalidate op 
          contractions.at(old_edge)->cont_op->cont = nullptr;
          // erase old contraction
          contractions.erase(old_edge);
        }  
        // remove duplicate neighbour entry
        neighbours[neighbour].erase(std::find(neighbours[neighbour].begin(), neighbours[neighbour].end(), old_id_2));  
      }
      neighbours.erase(old_id_1);
      neighbours.erase(old_id_2);
    }
    // insert new neighbours
    neighbours[new_id] = new_neighbours;
    // update queue, cheapest contraction at the back
    std::sort(contraction_queue.begin(), contraction_queue.end());
  }

  std::cout << "copying surfels" << std::endl;
  // save valid surfels in mem array
  surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);
  for (const auto& node : node_surfels) {
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
