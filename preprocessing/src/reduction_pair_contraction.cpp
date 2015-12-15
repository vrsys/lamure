// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_pair_contraction.h>
#include <lamure/pre/surfel.h>
#include <set>

namespace lamure {
namespace pre {

struct edge
{
  edge(surfel_id_t p1, surfel_id_t p2) 
   :a{p1.surfel_idx < p2.surfel_idx ? p1 : p2}
   ,b{p1.surfel_idx >= p2.surfel_idx ? p1 : p2}
   {};

   // operator<(const edge& e) const {
   //  return 
   // }

  surfel_id_t a;
  surfel_id_t b;
};

// namespace std {
//   template <>
//   struct hash<edge>
//   {
//     size_t operator()(edge const & e) const noexcept
//     {
//       return (e.a. * 100000 + e.b);
//     }
//   };
// }

size_t num_neighbours = 20;

surfel_mem_array reduction_pair_contraction::
create_lod(bvh* tree,
          real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node) const
{
  surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

  const uint32_t fan_factor = input.size();
  size_t num_points = 0;

  //compute max total number of surfels from all nodes
  for ( size_t node_idx = 0; node_idx < fan_factor; ++node_idx) {
    num_points += input[node_idx]->length();
  }

  std::vector<mat4r> quadrics{num_points};
  std::vector<std::vector<size_t>> neighbours{num_points, std::vector<size_t>{num_neighbours}};
  // std::unordered_set<edge> edges{};

  size_t offset = 0;
  for (node_id_type node_idx = 0; node_idx < fan_factor; ++node_idx) {
    for (size_t surfel_idx = 0; surfel_idx < input[node_idx]->length(); ++surfel_idx) {
      
      surfel curr_surfel = tree->nodes()[node_idx].mem_array().read_surfel(surfel_idx);
      surfel_id_t curr_id = surfel_id_t{node_idx, surfel_idx};

      auto nearest_neighbours = tree->get_nearest_neighbours(curr_id, num_neighbours);

      for (auto const& neighbour : nearest_neighbours) {
        edge curr_edge = edge(curr_id, neighbour.first); 
        // if (edges.find(curr_edge) == edges.end()) {
        //   edges.insert(curr_edge);
        // }
      }
    }
    offset += input[node_idx]->length();
  }

  mem_array.set_length(mem_array.mem_data()->size());

  reduction_error = 0.0;

  return mem_array;
}


lamure::mat4r edge_quadric(const vec3r& normal_p1, const vec3r& p1, const vec3r& p2)
{
    vec3r edge_dir = normalize(p2 - p1);
    vec3r tangent = normalize(cross(normal_p1, edge_dir));

    vec3r normal = normalize(cross(tangent, edge_dir));
    vec4r hessian = vec4r{normal, dot(p1, normal)}; 

    lamure::mat4r quadric = mat4r{hessian * hessian.x,
                                  hessian * hessian.y,
                                  hessian * hessian.z,
                                  hessian * hessian.w};

    return quadric;
}

lamure::real error(const vec3r& p, const mat4r& quadric)
{
  vec4r p_transformed = quadric * p;
  return dot(p, vec3r{p_transformed} / p_transformed.w);
}

} // namespace pre
} // namespace lamure
