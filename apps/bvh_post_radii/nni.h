
#ifndef NNI_H_INCLUDED
#define NNI_H_INCLUDED

#include <vector>
#include <map>

#include "types.h"
#include "plane.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/natural_neighbor_coordinates_2.h>

#include <ctime>
#include <cstdlib>
#include <algorithm>

namespace nni {

//#define NNI_VERBOSE

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_2 Point2;
typedef K::Vector_2 Vector2;
typedef CGAL::Delaunay_triangulation_2<K> Dh2;

typedef Dh2::Vertex_handle Vertex_handle;

struct nni_sample_t {
    scm::math::vec2f xy_;
    scm::math::vec2f uv_;
};

static void fit_plane(
    std::vector<std::pair<surfel, float>>& neighbours,
    plane_t& plane) {
    
    unsigned int num_neighbours = neighbours.size();

    scm::math::vec3f cen = scm::math::vec3f(0.f, 0.f, 0.f);

    for (const auto& it : neighbours) {
        cen.x += it.first.x;
        cen.y += it.first.y;
        cen.z += it.first.z;
    }
    
    scm::math::vec3f centroid = cen * (1.0f/ (float)num_neighbours);

    //calc covariance matrix
    scm::math::mat3f covariance_mat = scm::math::mat3f::zero();

    for (const auto& it : neighbours) {
        scm::math::vec3f c = scm::math::vec3f(it.first.x, it.first.y, it.first.z);

        covariance_mat.m00 += std::pow(c.x-centroid.x, 2);
        covariance_mat.m01 += (c.x-centroid.x) * (c.y - centroid.y);
        covariance_mat.m02 += (c.x-centroid.x) * (c.z - centroid.z);

        covariance_mat.m03 += (c.y-centroid.y) * (c.x - centroid.x);
        covariance_mat.m04 += std::pow(c.y-centroid.y, 2);
        covariance_mat.m05 += (c.y-centroid.y) * (c.z - centroid.z);

        covariance_mat.m06 += (c.z-centroid.z) * (c.x - centroid.x);
        covariance_mat.m07 += (c.z-centroid.z) * (c.y - centroid.y);
        covariance_mat.m08 += std::pow(c.z-centroid.z, 2);

    }

    //calculate normal
    scm::math::mat3f inv_covariance_mat = scm::math::inverse(covariance_mat);

    scm::math::vec3f first = scm::math::vec3f(1.0f, 1.0f, 1.0f);
    scm::math::vec3f second = scm::math::normalize(first*inv_covariance_mat);

    unsigned int iteration = 0;
    while (iteration++ < 512 && first != second) {
        first = second;
        second = scm::math::normalize(first*inv_covariance_mat);
    }
    
    plane = plane_t(second, centroid);

}

static void find_natural_neighbours(
    std::vector<std::pair<surfel, float>>& nearest_neighbours,
    const scm::math::vec3f& point_of_interest,
    std::vector<std::pair<surfel, float>>& natural_neighbours) {

    std::srand(613475635);
    std::random_shuffle(nearest_neighbours.begin(), nearest_neighbours.end());
    
    
    unsigned int num_neighbours = nearest_neighbours.size();

    //compile points
    float* points = new float[num_neighbours*3];
    for (unsigned int i = 0; i < num_neighbours; ++i) {
        points[3*i+0] = nearest_neighbours[i].first.x;
        points[3*i+1] = nearest_neighbours[i].first.y;
        points[3*i+2] = nearest_neighbours[i].first.z;
    }
    
    //compute best fit plane
    plane_t plane;
    fit_plane(nearest_neighbours, plane);

    bool is_projection_valid = true;

    //project all points to the plane
    float* coords = new float[2*num_neighbours];
    scm::math::vec3f plane_right = plane.get_right();
    for (unsigned int i = 0; i < num_neighbours; ++i) {
        scm::math::vec3f v = scm::math::vec3f(points[3*i+0], points[3*i+1], points[3*i+2]);
        scm::math::vec2f c = plane_t::project(plane, plane_right, v);
        coords[2*i+0] = c.x;
        coords[2*i+1] = c.y;
        
        if (c.x != c.x || c.y != c.y) { //is nan?
            is_projection_valid = false;
        }
    }
    
    if (!is_projection_valid) {
        std::cout << "invalid projection encountered" << std::endl;
        natural_neighbours.clear();
        return;
    }
    
#ifdef NNI_VERBOSE
    std::cout << "reparameterization: " << std::endl;
    for (unsigned int i = 0; i < num_neighbours; ++i) {
        
        scm::math::vec3f v = scm::math::vec3f(points[3*i+0], points[3*i+1], points[3*i+2]);
        std::cout << i << ": ("<< v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
        
        scm::math::vec2f c = scm::math::vec2f(coords[2*i+0], coords[2*i+1]);
        std::cout << i << ": ("<< c.x << ", " << c.y << ")" << std::endl;
    }
#endif

    //project point of interest
    scm::math::vec2f coord_poi = plane_t::project(plane, plane_right, point_of_interest);
    
#ifdef NNI_VERBOSE
    std::cout << "poi in plane: (" << coord_poi.x << ", " << coord_poi.y << ")" << std::endl;
#endif

    //cgal delaunay triangluation
    Dh2 delaunay_triangulation;
    //std::map<Vertex_handle, nni_sample_t> nni_handles;
    
#ifdef NNI_VERBOSE
    std::cout << "construct delaunay triangulation" << std::endl;
#endif

    for (unsigned int i = 0; i < num_neighbours; ++i) {
        nni_sample_t sp;
        sp.xy_ = scm::math::vec2f(coords[2*i+0], coords[2*i+1]);
        Point2 p(sp.xy_.x, sp.xy_.y);
        //nni_handles[delaunay_triangulation.insert(p)] = sp;
        delaunay_triangulation.insert(p);
    }
    
    Point2 poi_2d(coord_poi.x, coord_poi.y);
    
    std::vector<std::pair<K::Point_2, K::FT>> sibson_coords;
    //float sibson_norm_coeff;
    CGAL::Triple<std::back_insert_iterator<std::vector<std::pair<K::Point_2, K::FT>>>, K::FT, bool> result = 
        natural_neighbor_coordinates_2(
            delaunay_triangulation,
            poi_2d,
            std::back_inserter(sibson_coords)/*,
            sibson_norm_coeff*/);

#ifdef NNI_VERBOSE
    std::cout << "num natural neighbours: " << sibson_coords.size() << std::endl;
#endif
    if (!result.third) {
#ifdef NNI_VERBOSE
        std::cout << "natural neighbour computation was not successful!" << std::endl;
#endif
        natural_neighbours.clear();
        return;
    }

    std::map<unsigned int, float> nni_weights;
    for (const auto& it : sibson_coords) {
        
        for (unsigned int i = 0; i < num_neighbours; ++i) {
            if (coords[2*i+0] == it.first.x() && coords[2*i+1] == it.first.y()) {
                nni_weights[i] = (float)it.second;
#ifdef NNI_VERBOSE
                std::cout << i << " : (" << it.second << ")" << std::endl;
#endif
                break;
            }
        
        }
    }
    
    for (const auto& it : nni_weights) {
        const surfel surfel = nearest_neighbours[it.first].first;
        natural_neighbours.push_back(std::make_pair(surfel, it.second));
    }
#ifdef NNI_VERBOSE
    if (natural_neighbours.size() != sibson_coords.size()) {
        std::cout << "WARNING! could not locate all sibson weights! (" << natural_neighbours.size() << ", " << sibson_coords.size() << ") " << std::endl;
    }
#endif
    
    delete[] points;
    delete[] coords;
    
    sibson_coords.clear();
    nni_weights.clear();
    
}




}



#endif
