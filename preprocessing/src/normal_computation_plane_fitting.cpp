// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>

#include <lamure/pre/bvh.h>
#include <lamure/pre/normal_computation_plane_fitting.h>

namespace lamure {
namespace pre{

vec3f normal_computation_plane_fitting::
compute_normal(const bvh& tree,
			   const surfel_id_t target_surfel) const {

    const uint16_t num = number_of_neighbours_;
	// find nearest neighbours
    std::vector<std::pair<surfel_id_t, real>> neighbours = tree.get_nearest_neighbours(target_surfel.node_idx, target_surfel.surfel_idx, num);

    auto& bvh_nodes = (tree.nodes());
    surfel surf = bvh_nodes[target_surfel.node_idx].mem_array().read_surfel(target_surfel.surfel_idx);

    // compute normal
    // see: http://missingbytes.blogspot.com/2012/06/fitting-plane-to-point-cloud.html
    vec3r center = surf.pos();

    vec3f normal(1.0, 0.0 , 0.0);

    real sum_x_x = 0.0, sum_x_y = 0.0, sum_x_z = 0.0;
    real sum_y_y = 0.0, sum_y_z = 0.0;
    real sum_z_z = 0.0;

    for (auto neighbour : neighbours) {
        vec3r neighbour_pos = bvh_nodes[neighbour.first.node_idx].mem_array().read_surfel(neighbour.first.surfel_idx).pos();
    	// vec3r neighbour_pos = neighbour.first.pos();

    	real diff_x = neighbour_pos.x - center.x;
    	real diff_y = neighbour_pos.y - center.y;
    	real diff_z = neighbour_pos.z - center.z;

    	sum_x_x += diff_x * diff_x;
    	sum_x_y += diff_x * diff_y;
    	sum_x_z += diff_x * diff_z;
    	sum_y_y += diff_y * diff_y;
    	sum_y_z += diff_y * diff_z;
    	sum_z_z += diff_z * diff_z;
    }

    scm::math::mat3f matrix;

    matrix[0] = sum_x_x;
    matrix[1] = sum_x_y;
    matrix[2] = sum_x_z;

    matrix[3] = sum_x_y;
    matrix[4] = sum_y_y;
    matrix[5] = sum_y_z;

    matrix[6] = sum_x_z;
    matrix[7] = sum_y_z;
    matrix[8] = sum_z_z;

    if (!(scm::math::determinant(matrix) == 0.0f)) {
    	matrix = scm::math::inverse(matrix);

    	float scale = fabs(matrix[0]);
    	for (uint16_t i = 1; i < 9; ++i)
    	{
    		scale = std::max(scale, float(fabs(matrix[i])));
    	}

    	scm::math::mat3f matrix_c = matrix * (1.0f/scale);

    	vec3f vector(1.0f, 1.0f, 1.0f);
    	vec3f last_vector = vector;

    	for (uint16_t i = 0; i < 100; i++) {
    		vector = (matrix_c * vector);
    		vector = scm::math::normalize(vector);

    		if (pow(scm::math::length(vector - last_vector), 2) < 1e-16f) {
    			break;
    		}
    		last_vector = vector;
    	}

    	normal = vector;
    }
	
		return  normal;
	};

}// namespace pre
}// namespace lamure



