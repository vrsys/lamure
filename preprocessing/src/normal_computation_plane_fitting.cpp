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

void normal_computation_plane_fitting::
eigsrt_jacobi(
    int dim,
    double* eigenvalues, 
    double** eigenvectors) const{

    for (int i = 0; i < dim; ++i)  {
        int k = i;
        double v = eigenvalues[k];
        for (int j = i + 1; j < dim; ++j) {
            if (eigenvalues[j] < v) { // eigenvalues[j] <= v ?
                k = j;
                v = eigenvalues[k];
            }
        }
        if (k != i)  {
            eigenvalues[k] = eigenvalues[i];
            eigenvalues[i] = v;
            for (int j = 0; j < dim; ++j)  {
                v = eigenvectors[j][i];
                eigenvectors[j][i] = eigenvectors[j][k];
                eigenvectors[j][k] = v;
            }
        }
    }

}

void normal_computation_plane_fitting::
jacobi_rotation(const scm::math::mat3d& _matrix,
                     double* eigenvalues,
                     double** eigenvectors) const {

    unsigned int max_iterations = 10;
    double max_error = 0.00000001;
    unsigned int dim = 3;

    double iR1[3][3];
    iR1[0][0] = _matrix[0];
    iR1[0][1] = _matrix[1];
    iR1[0][2] = _matrix[2];
    iR1[1][0] = _matrix[3];
    iR1[1][1] = _matrix[4];
    iR1[1][2] = _matrix[5];
    iR1[2][0] = _matrix[6];
    iR1[2][1] = _matrix[7];
    iR1[2][2] = _matrix[8];


    for (uint32_t i = 0; i <= dim-1; ++i){
        eigenvectors[i][i] = 1.0;
        for (uint32_t j = 0; j <= dim-1; ++j){
            if (i != j) {
               eigenvectors[i][j] = 0.0;
            }
        }
    }

    uint32_t iteration = 0;
    uint32_t p, q;

    while (iteration < max_iterations){

        double max_diff = 0.0;

        for (uint32_t i = 0; i < dim-1 ; ++i){
            for (uint32_t j = i+1; j < dim ; ++j){
                long double diff = std::abs(iR1[i][j]);
                if (i != j && diff >= max_diff){
                    max_diff = diff;
                    p = i;
                    q = j;
                }
            }
        }

        if (max_diff < max_error){
            for (uint32_t i = 0; i < dim; ++i){
                eigenvalues[i] = iR1[i][i];
            }
            break;
        }

        long double x = -iR1[p][q];
        long double y = (iR1[q][q] - iR1[p][p]) / 2.0;
        long double omega = x / sqrt(x * x + y * y);

        if (y < 0.0) {
            omega = -omega;
        }

        long double sn = 1.0 + sqrt(1.0 - omega*omega);
        sn = omega / sqrt(2.0 * sn);
        long double cn = sqrt(1.0 - sn*sn);

        max_diff = iR1[p][p];

        iR1[p][p] = max_diff*cn*cn + iR1[q][q]*sn*sn + iR1[p][q]*omega;
        iR1[q][q] = max_diff*sn*sn + iR1[q][q]*cn*cn - iR1[p][q]*omega;
        iR1[p][q] = 0.0;
        iR1[q][p] = 0.0;

        for (uint32_t j = 0; j <= dim-1; ++j){
            if (j != p && j != q){
                max_diff = iR1[p][j];
                iR1[p][j] = max_diff*cn + iR1[q][j]*sn;
                iR1[q][j] = -max_diff*sn + iR1[q][j]*cn;
            }
        }

        for (uint32_t i = 0; i <= dim-1; ++i){
            if (i != p && i != q){
                max_diff = iR1[i][p];
                iR1[i][p] = max_diff*cn + iR1[i][q]*sn;
                iR1[i][q] = -max_diff*sn + iR1[i][q]*cn;
            }
        }

        for (uint32_t i = 0; i <= dim-1; ++i){
            max_diff = eigenvectors[i][p];
            eigenvectors[i][p] = max_diff*cn + eigenvectors[i][q]*sn;
            eigenvectors[i][q] = -max_diff*sn + eigenvectors[i][q]*cn;
        }

        ++iteration;
    }

    
    eigsrt_jacobi(dim, eigenvalues, eigenvectors);

}

vec3f normal_computation_plane_fitting::
compute_normal(const bvh& tree,
			   const surfel_id_t target_surfel,
               std::vector<std::pair<surfel_id_t, real>> const& nearest_neighbours) const {

    const uint16_t num = number_of_neighbours_;
	// find nearest neighbours
    std::vector<std::pair<surfel_id_t, real>> nearest_neighbours_ids;// = tree.get_nearest_neighbours(target_surfel, num);


    unsigned processed_neighbour_counter = 0;
    // compute radius
    for (auto const& neighbour : nearest_neighbours) {
        if( processed_neighbour_counter++ < number_of_neighbours_) {
            nearest_neighbours_ids.emplace_back(nearest_neighbours[processed_neighbour_counter-1]);
        }
        else {
            break;
        }
    }


    auto& bvh_nodes = (tree.nodes());
    surfel surf = bvh_nodes[target_surfel.node_idx].mem_array().read_surfel(target_surfel.surfel_idx);

    vec3r poi = surf.pos();

    uint32_t num_neighbours = nearest_neighbours_ids.size();
    if (num_neighbours < 3) {
        return vec3f(0.0,0.0,0.0);
    }

    double summed_distance = 0.0f;
    double max_distance = 0.0f; 
    vec3r cen = vec3r(0.0, 0.0, 0.0);

    for (const auto& neighbour_ids : nearest_neighbours_ids) {

        

        vec3r const& neighbour_pos = bvh_nodes[neighbour_ids.first.node_idx].mem_array().read_surfel(neighbour_ids.first.surfel_idx).pos();
        if (neighbour_pos == poi) {
            continue;
        }

        cen += neighbour_pos;
    
        real distance = scm::math::length(poi - neighbour_pos);
        summed_distance += distance;
        max_distance = std::max(distance, max_distance);
    }

    scm::math::vec3d centroid = cen * (1.0/ (real)num_neighbours);

    if (max_distance >= 0.01) {
    //    return false;
    }

    //produce covariance matrix
    scm::math::mat3d covariance_mat = scm::math::mat3d::zero();

    for (const auto& neighbour_ids : nearest_neighbours_ids) {
        vec3r const& neighbour_pos = bvh_nodes[neighbour_ids.first.node_idx].mem_array().read_surfel(neighbour_ids.first.surfel_idx).pos();
        if (neighbour_pos == poi) {
            continue;
        }
        
        covariance_mat.m00 += std::pow(neighbour_pos.x-centroid.x, 2);
        covariance_mat.m01 += (neighbour_pos.x-centroid.x) * (neighbour_pos.y - centroid.y);
        covariance_mat.m02 += (neighbour_pos.x-centroid.x) * (neighbour_pos.z - centroid.z);

        covariance_mat.m03 += (neighbour_pos.y-centroid.y) * (neighbour_pos.x - centroid.x);
        covariance_mat.m04 += std::pow(neighbour_pos.y-centroid.y, 2);
        covariance_mat.m05 += (neighbour_pos.y-centroid.y) * (neighbour_pos.z - centroid.z);

        covariance_mat.m06 += (neighbour_pos.z-centroid.z) * (neighbour_pos.x - centroid.x);
        covariance_mat.m07 += (neighbour_pos.z-centroid.z) * (neighbour_pos.y - centroid.y);
        covariance_mat.m08 += std::pow(neighbour_pos.z-centroid.z, 2);

    }

    //solve for eigenvectors
    real* eigenvalues = new real[3];
    real** eigenvectors = new real*[3];
    for (int i = 0; i < 3; ++i) {
       eigenvectors[i] = new real[3];
    }

    jacobi_rotation(covariance_mat, eigenvalues, eigenvectors);

    scm::math::vec3f normal = scm::math::vec3f(eigenvectors[0][0], 
                                               eigenvectors[1][0], 
                                               eigenvectors[2][0]);
    //scm::math::vec3f binormal = scm::math::vec3f(eigenvectors[0][1], eigenvectors[1][1], eigenvectors[2][1]);
    //scm::math::vec3f tangent = scm::math::vec3f(eigenvectors[0][2], eigenvectors[1][2], eigenvectors[2][2]);

    delete[] eigenvalues;
    for (int i = 0; i < 3; ++i) {
       delete[] eigenvectors[i];
    }
    delete[] eigenvectors;

	return  normal;
}

}// namespace pre
}// namespace lamure



