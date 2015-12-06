// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef NORMALS_H_INCLUDED
#define NORMALS_H_INCLUDED

#include <iostream>
#include <vector>
#include <algorithm>
#include "types.h"
#include "node_queue.h"
#include <scm/core/math.h>

namespace nrm {

void eigsrt_jacobi(
    int dim,
    double* eigenvalues, 
    double** eigenvectors) {

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

void jacobi_rotation(const scm::math::mat3d& _matrix,
                     double* eigenvalues,
                     double** eigenvectors) {

    unsigned int max_iterations = 10;
    double max_error = 0.00000001;
    unsigned int dim = 3;

    //double iR1[3][3] = {  {_matrix.getElement(0,0),_matrix.getElement(0,1),m.getElement(0,2)},
    //                      {_matrix.getElement(1,0),_matrix.getElement(1,1),m.getElement(1,2)},
    //                      {_matrix.getElement(2,0),_matrix.getElement(2,1),m.getElement(2,2)}
    //                   };

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


    for (int i = 0; i <= dim-1; ++i){
        eigenvectors[i][i] = 1.0;
        for (int j = 0; j <= dim-1; ++j){
            if (i != j) {
               eigenvectors[i][j] = 0.0;
            }
        }
    }

    unsigned int iteration = 0;
    int p, q;

    while (iteration < max_iterations){

        double max_diff = 0.0;

        for (int i = 0; i < dim-1 ; ++i){
            for (int j = i+1; j < dim ; ++j){
                long double diff = std::abs(iR1[i][j]);
                if (i != j && diff >= max_diff){
                    max_diff = diff;
                    p = i;
                    q = j;
                }
            }
        }

        if (max_diff < max_error){
            for (int i = 0; i < dim; ++i){
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

        for (int j = 0; j <= dim-1; ++j){
            if (j != p && j != q){
                max_diff = iR1[p][j];
                iR1[p][j] = max_diff*cn + iR1[q][j]*sn;
                iR1[q][j] = -max_diff*sn + iR1[q][j]*cn;
            }
        }

        for (int i = 0; i <= dim-1; ++i){
            if (i != p && i != q){
                max_diff = iR1[i][p];
                iR1[i][p] = max_diff*cn + iR1[i][q]*sn;
                iR1[i][q] = -max_diff*sn + iR1[i][q]*cn;
            }
        }

        for (int i = 0; i <= dim-1; ++i){
            max_diff = eigenvectors[i][p];
            eigenvectors[i][p] = max_diff*cn + eigenvectors[i][q]*sn;
            eigenvectors[i][q] = -max_diff*sn + eigenvectors[i][q]*cn;
        }

        ++iteration;
    }

    //std::cout << iteration << " sweeps needed to reduce error lower than " << epsilon << std::endl;
    
    eigsrt_jacobi(dim, eigenvalues, eigenvectors);

}

bool calculate_normal(
    std::vector<std::pair<surfel, float>>& nearest_neighbours,
    const scm::math::vec3f& point_of_interest,
    scm::math::vec3f* out_normal) {

    scm::math::vec3d poi = scm::math::vec3d(point_of_interest.x, point_of_interest.y, point_of_interest.z);
    unsigned int num_neighbours = nearest_neighbours.size();
    if (num_neighbours < 3) {
        return false;
    }

    double summed_distance = 0.0f;
    double max_distance = 0.0f;	
    scm::math::vec3d cen = scm::math::vec3d(0.0, 0.0, 0.0);

    for (const auto& it : nearest_neighbours) {
        scm::math::vec3d p = scm::math::vec3d(it.first.x, it.first.y, it.first.z);
        if (p == poi) {
            continue;
        }

        cen += p;
    
        double distance = scm::math::length(poi - p);
        summed_distance += distance;
        max_distance = std::max(distance, max_distance);
    }

    scm::math::vec3d centroid = cen * (1.0/ (double)num_neighbours);

    if (max_distance >= 0.01) {
    //    return false;
    }

    //produce covariance matrix
    scm::math::mat3d covariance_mat = scm::math::mat3d::zero();

    for (const auto& it : nearest_neighbours) {
        scm::math::vec3d p = scm::math::vec3d(it.first.x, it.first.y, it.first.z);
        if (p == poi) {
            continue;
        }
        
        covariance_mat.m00 += std::pow(p.x-centroid.x, 2);
        covariance_mat.m01 += (p.x-centroid.x) * (p.y - centroid.y);
        covariance_mat.m02 += (p.x-centroid.x) * (p.z - centroid.z);

        covariance_mat.m03 += (p.y-centroid.y) * (p.x - centroid.x);
        covariance_mat.m04 += std::pow(p.y-centroid.y, 2);
        covariance_mat.m05 += (p.y-centroid.y) * (p.z - centroid.z);

        covariance_mat.m06 += (p.z-centroid.z) * (p.x - centroid.x);
        covariance_mat.m07 += (p.z-centroid.z) * (p.y - centroid.y);
        covariance_mat.m08 += std::pow(p.z-centroid.z, 2);

    }

    //solve for eigenvectors
    double* eigenvalues = new double[3];
    double** eigenvectors = new double*[3];
    for (int i = 0; i < 3; ++i) {
       eigenvectors[i] = new double[3];
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

    (*out_normal) = normal;

    return true;

}


}

#endif
