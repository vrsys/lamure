// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/natural_neighbor_coordinates_2.h>

#include <lamure/pre/bvh.h>
#include <lamure/pre/radius_computation_natural_neighbours.h>
#include <lamure/pre/plane.h>

#include <iostream>

namespace lamure {
namespace pre{

namespace nni {

using K       = CGAL::Exact_predicates_inexact_constructions_kernel;
using Point2  = K::Point_2;
using Vector2 = K::Vector_2;
using Dh2     = CGAL::Delaunay_triangulation_2<K>;

struct nni_sample_t {
    scm::math::vec2f xy_;
    scm::math::vec2f uv_;
};


static void find_natural_neighbours(
    bvh const& tree,
    std::vector<std::pair<surfel_id_t, real>> & nearest_neighbour_ids,
    vec3r const & point_of_interest,
    std::vector<std::pair<surfel, double>>& natural_neighbours) {

    std::srand(613475635);
    std::random_shuffle(nearest_neighbour_ids.begin(), nearest_neighbour_ids.end());
    
    
    unsigned int num_neighbours = nearest_neighbour_ids.size();

    //compile points
    double* points = new double[num_neighbours*3];

    auto const& bvh_nodes = tree.nodes();

    std::vector<vec3r> nn_positions;

    nn_positions.reserve(nearest_neighbour_ids.size());

    uint32_t point_counter = 0;

    std::vector<surfel> nearest_neighbour_copies;
    for (auto const& idx_pair : nearest_neighbour_ids) {

        surfel const current_nearest_neighbour_surfel = 
            bvh_nodes[idx_pair.first.node_idx].mem_array().read_surfel(idx_pair.first.surfel_idx);
        auto const& neighbour_pos 
            = current_nearest_neighbour_surfel.pos();

        nearest_neighbour_copies.push_back(current_nearest_neighbour_surfel);


        nn_positions.push_back( neighbour_pos );

        points[3*point_counter+0] = neighbour_pos[0];
        points[3*point_counter+1] = neighbour_pos[1];
        points[3*point_counter+2] = neighbour_pos[2];

        ++point_counter;
    }
    
    //compute best fit plane
    plane_t plane;
    plane_t::fit_plane(nn_positions, plane);

    bool is_projection_valid = true;

    //project all points to the plane
    double* coords = new double[2*num_neighbours];
    scm::math::vec3f plane_right = plane.get_right();
    for (unsigned int i = 0; i < num_neighbours; ++i) {
        vec3r v = vec3r(points[3*i+0], points[3*i+1], points[3*i+2]);
        vec2r c = plane_t::project(plane, plane_right, v);
        coords[2*i+0] = c[0];
        coords[2*i+1] = c[1];

        if (c[0] != c[0] || c[1] != c[1]) { //is nan?
            is_projection_valid = false;
        }
    }
    
    if (!is_projection_valid) {
        natural_neighbours.clear();
        delete[] points;
        delete[] coords;

        return;
    }


    //project point of interest
    vec2r coord_poi = plane_t::project(plane, plane_right, point_of_interest);
    

    //cgal delaunay triangluation
    Dh2 delaunay_triangulation;


    std::vector<scm::math::vec2f> neighbour_2d_coord_pairs;

    for (unsigned int i = 0; i < num_neighbours; ++i) {
        nni_sample_t sp;
        sp.xy_ = scm::math::vec2f(coords[2*i+0], coords[2*i+1]);
        Point2 p(sp.xy_.x, sp.xy_.y);
        delaunay_triangulation.insert(p);
        neighbour_2d_coord_pairs.emplace_back(coords[2*i+0], coords[2*i+1]);
    }
    
    Point2 poi_2d(coord_poi.x, coord_poi.y);
    
    std::vector<std::pair<K::Point_2, K::FT>> sibson_coords;
    CGAL::Triple<std::back_insert_iterator<std::vector<std::pair<K::Point_2, K::FT>>>, K::FT, bool> result = 
        natural_neighbor_coordinates_2(
            delaunay_triangulation,
            poi_2d,
            std::back_inserter(sibson_coords));


    if (!result.third) {    
        natural_neighbours.clear();
        delete[] points;
        delete[] coords;
        return;
    }

    // first  := nearest_neighbour id
    // second := natural_neighbour weight
    std::vector<std::pair<unsigned int, double> > nni_weights;

    unsigned nn_counter = 0;

    for (const auto& sibs_coord_instance : sibson_coords) {
        uint32_t closest_nearest_neighbour_id = std::numeric_limits<uint32_t>::max();
        double min_distance = std::numeric_limits<double>::max();

        uint32_t current_neighbour_id = 0;
        for( auto const& nearest_neighbour_2d : neighbour_2d_coord_pairs) {
            double current_distance = scm::math::length(nearest_neighbour_2d - scm::math::vec2f(sibs_coord_instance.first.x(),
                                                                                                sibs_coord_instance.first.y()) );
            if( current_distance < min_distance ) {
                min_distance = current_distance;
                closest_nearest_neighbour_id = current_neighbour_id;
            }

            ++current_neighbour_id;
        }

        //invalidate the 2d coord pair by putting ridiculously large 2d coords that the model is unlikely to contain
        neighbour_2d_coord_pairs[closest_nearest_neighbour_id] = scm::math::vec2f( std::numeric_limits<float>::max(), 
                                                                                   std::numeric_limits<float>::lowest() );

        nni_weights.emplace_back( closest_nearest_neighbour_id, (double) sibs_coord_instance.second );
    }
    

    
    for (const auto& it : nni_weights) {
        surfel const& curr_surfel = nearest_neighbour_copies[it.first] ;
        natural_neighbours.push_back(std::make_pair(curr_surfel, it.second));
    }

    delete[] points;
    delete[] coords;
    
    sibson_coords.clear();
    nni_weights.clear();
}




}


real radius_computation_natural_neighbours::
compute_radius(const bvh& tree,
			   const surfel_id_t target_surfel) const {
	
	// find nearest neighbours
	std::vector<std::pair<surfel_id_t, real>> nearest_neighbours 
        = tree.get_nearest_neighbours(target_surfel, desired_num_nearest_neighbours_);
    
                 
    if (nearest_neighbours.size() < min_num_nearest_neighbours_) {
         return 0.0f;
    }
    

    auto const& current_node = (tree.nodes()[target_surfel.node_idx]);
    vec3r poi = current_node.mem_array().read_surfel(target_surfel.surfel_idx).pos();

    std::vector<std::pair<surfel, double>> natural_neighbours; //surfel + weight
    
    nni::find_natural_neighbours(tree,
                                 nearest_neighbours, 
                                 poi, 
                                 natural_neighbours);

    if (natural_neighbours.size() < min_num_natural_neighbours_) {
        return 0.0;
    }

    //determine most distant natural neighbour
    real max_distance = 0.f;
    for (const auto& nn : natural_neighbours) {

        auto const& surfel_pos = nn.first.pos();
        real new_dist = scm::math::length_sqr( poi-( surfel_pos ) );
        max_distance = std::max(max_distance, new_dist);
    }

    if (max_distance >= std::numeric_limits<real>::min()) {
        max_distance = scm::math::sqrt(max_distance);
        //std::cout << "returning: " << 0.5*max_distance;
        return 0.5 * max_distance;

        //out_surfels[job.job_id_*bvh->surfels_per_node() + splat_id].size = 0.5f*max_distance;
    } else { 
        return 0.0;
    }

    natural_neighbours.clear();

	};

}// namespace pre
}// namespace lamure