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
  edge(size_t p1, size_t p2) 
   :a{p1 < p2 ? p1 : p2}
   ,b{p1 > p2 ? p1 : p2}
   {};

  size_t a;
  size_t b;
};

size_t num_neighbours =20;

surfel_mem_array reduction_pair_contraction::
create_lod(bvh* tree,
          real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const uint32_t surfels_per_node) const
{
  surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

  const uint32_t fan_factor = input.size();
  size_t surfel_id = 0;

  size_t num_points = 0;

  //compute max total number of surfels from all nodes
  for ( size_t node_id = 0; node_id < fan_factor; ++node_id) {
    num_points += input[node_id]->length();
  }

  std::vector<mat4r> quadrics{num_points};
  std::vector<std::vector<size_t>> neighbours{num_points, std::vector<size_t>{num_neighbours}};
  std::set<edge> edges{};

  size_t offset = 0;
  for (size_t node_id = 0; node_id < fan_factor; ++node_id) {
    for (size_t surfel_id = 0; surfel_id < input[node_id]->length(); ++surfel_id) {
      std::vector<std::pair<surfel, real>> neighbours = tree->get_nearest_neighbours(
                            node_id,
                            surfel_id,
                            num_neighbours);
      for( auto const& neighbour : neighbours) {
        // if (edges.find(edge(surfel_id + input[node_id]->length(), neighbour.first)))
          
        // }
      }
    }
    offset += input[node_id]->length();
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
