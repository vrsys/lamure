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
#include <lamure/pre/bvh_stream.h>
#include <lamure/pre/basic_algorithms.h>
#include <lamure/pre/serialized_surfel.h>
#include <lamure/pre/plane.h>
#include <lamure/atomic_counter.h>
#include <lamure/utils.h>
#include <lamure/sphere.h>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <limits>
#include <math.h>
#include <memory>
#include <map>
#include <set>
#include <unordered_set>

#if WIN32
  #include <io.h>
  #include <ppl.h>
#else
  #include <parallel/algorithm>
#endif


#include <sys/stat.h>
#include <fcntl.h>

namespace fs = boost::filesystem;

namespace lamure {
namespace pre {

class reduction_strategy;

using K       = CGAL::Exact_predicates_inexact_constructions_kernel;
using Point2  = K::Point_2;
using Vector2 = K::Vector_2;
using Dh2     = CGAL::Delaunay_triangulation_2<K>;

struct nni_sample_t {
    scm::math::vec2f xy_;
    scm::math::vec2f uv_;
};

void  bvh::
init_tree(const std::string& surfels_input_file,
          const uint32_t max_fan_factor,
          const size_t desired_surfels_per_node,
          const boost::filesystem::path& base_path)
{
    assert(state_ == state_type::null);
    assert(max_fan_factor >= 2);
    assert(desired_surfels_per_node >= 5);

    base_path_ = base_path;

    // get number of surfels
    file input;
    input.open(surfels_input_file);
    size_t num_surfels = input.get_size();
    input.close();

    // compute bvh properties
    size_t best = std::numeric_limits<size_t>::max();
    for (size_t i = 2; i <= max_fan_factor; ++i) {
        size_t depth = std::round(std::log(num_surfels / desired_surfels_per_node)
                                  / std::log(i));
        size_t num_leaves = std::round(std::exp(depth*std::log(i)));
        int64_t temp_max_surfels_per_node = std::ceil(double(num_surfels)
                                                     / double(num_leaves));

        size_t diff = std::abs(int64_t(desired_surfels_per_node)
                               - temp_max_surfels_per_node);

        if (diff < best) {
            best = diff;
            fan_factor_ = i;
            depth_ = depth;
            max_surfels_per_node_ = temp_max_surfels_per_node;
        }
    }

    // compute number of nodes
    size_t num_nodes = 1, count = 1;
    for (uint32_t i = 1; i <= depth_; ++i) {
        num_nodes += count *= fan_factor_;
    }

    nodes_ = std::vector<bvh_node>(num_nodes);
    first_leaf_ = nodes_.size() - std::pow(fan_factor_, depth_);
    state_ = state_type::empty;

    std::srand(613475635);
}

bool bvh::
load_tree(const std::string& kdn_input_file)
{
    assert(state_ == state_type::null);

    bvh_stream bvh_strm;
    bvh_strm.read_bvh(kdn_input_file, *this);

    LOGGER_INFO("Load bvh: \"" << kdn_input_file << "\". state_type: " << state_to_string(state_));
    return true;
}

uint32_t bvh::
get_depth_of_node(const uint32_t node_id) const{
    uint32_t node_depth = 0;

    uint32_t current_node_id = node_id;
    while(current_node_id != 0) {
        ++node_depth;
        current_node_id = get_parent_id(current_node_id);
    }

    return node_depth;
}

uint32_t bvh::
get_child_id(const uint32_t node_id, const uint32_t child_index) const
{
    return node_id*fan_factor_ + 1 + child_index;
}

uint32_t bvh::
get_parent_id(const uint32_t node_id) const
{
    //TODO: might be better to assert on root node instead
    if (node_id == 0) return 0;

    if (node_id % fan_factor_ == 0)
        return node_id/fan_factor_ - 1;
    else
        return (node_id + fan_factor_ - (node_id % fan_factor_))
               / fan_factor_ - 1;
}

const node_t bvh::
get_first_node_id_of_depth(uint32_t depth) const {
    node_t id = 0;
    for (uint32_t i = 0; i < depth; ++i) {
        id += (node_t)pow((double)fan_factor_, (double)i);
    }

    return id;
}

const uint32_t bvh::
get_length_of_depth(uint32_t depth) const {
    return pow((double)fan_factor_, (double)depth);
}

std::pair<node_id_type, node_id_type> bvh::
get_node_ranges(const uint32_t depth) const
{
    assert(depth >= 0 && depth <= depth_);

    node_id_type first = 0, count = 1;
    for (node_id_type i = 1; i <= depth; ++i) {
        first += count;
        count *= fan_factor_;
    }
    return std::make_pair(first, count);
}

void bvh::
print_tree_properties() const
{
    LOGGER_INFO("Fan-out factor: " << int32_t(fan_factor_));
    LOGGER_INFO("Depth: " << depth_);
    LOGGER_INFO("Number of nodes: " << nodes_.size());
    LOGGER_INFO("Max surfels per node: " << max_surfels_per_node_);
    LOGGER_INFO("First leaf node id: " << first_leaf_);
}

void bvh::
downsweep(bool adjust_translation,
          const std::string& surfels_input_file,
          bool bin_all_file_extension)
{
    assert(state_ == state_type::empty);

    size_t in_core_surfel_capacity = memory_limit_ / sizeof(surfel);

    size_t disk_leaf_destination = 0,
           slice_left = 0,
           slice_right = 0;

    LOGGER_INFO("Build bvh for \"" << surfels_input_file << "\"");

    // open input file and leaf level file
    shared_file input_file_disk_access = std::make_shared<file>();
    input_file_disk_access->open(surfels_input_file);

    shared_file leaf_level_access = std::make_shared<file>();
    std::string file_extension = ".lv" + std::to_string(depth_);
    if (bin_all_file_extension)
        file_extension = ".bin_all";
    leaf_level_access->open(add_to_path(base_path_, file_extension).string(), true);

    // instantiate root surfel array
    surfel_disk_array input(input_file_disk_access, 0, input_file_disk_access->get_size());
    LOGGER_INFO("Total number of surfels: " << input.length());

    // compute depth at which we can switch to in-core
    uint32_t final_depth = std::max(0.0,
            std::ceil(std::log(input.length() / double(in_core_surfel_capacity)) /
                 std::log(double(fan_factor_))));

    assert(final_depth <= depth_);

    LOGGER_INFO("Tree depth to switch in-core: " << final_depth);

    // construct root node
    nodes_[0] = bvh_node(0, 0, bounding_box(), input);
    bounding_box input_bb;

    // check if the root can be switched to in-core
    if (final_depth == 0) {
        LOGGER_TRACE("Compute root bounding box in-core");
        nodes_[0].load_from_disk();
        input_bb = basic_algorithms::compute_aabb(nodes_[0].mem_array());

    }
    else {
        LOGGER_TRACE("Compute root bounding box out-of-core");
        input_bb = basic_algorithms::compute_aabb(nodes_[0].disk_array(),
                                                  buffer_size_);
    }
    LOGGER_DEBUG("Root AABB: " << input_bb.min() << " - " << input_bb.max());

    // translate all surfels by the root AABB center
    if (adjust_translation) {
        vec3r translation = (input_bb.min() + input_bb.max()) * vec3r(0.5);
        translation.x = std::floor(translation.x);
        translation.y = std::floor(translation.y);
        translation.z = std::floor(translation.z);
        translation_ = translation;

        LOGGER_INFO("The surfels will be translated by: " << translation);

        input_bb.min() -= translation;
        input_bb.max() -= translation;

        if (final_depth == 0) {
            basic_algorithms::translate_surfels(nodes_[0].mem_array(), -translation);
        }
        else {
            basic_algorithms::translate_surfels(nodes_[0].disk_array(), -translation, buffer_size_);
        }
        LOGGER_DEBUG("New root AABB: " << input_bb.min() << " - " << input_bb.max());
    }
    else {
        translation_ = vec3r(0.0);
    }

    nodes_[0].set_bounding_box(input_bb);

    // construct out-of-core

    uint32_t processed_nodes = 0;
    uint8_t percent_processed = 0;
    for (uint32_t level = 0; level < final_depth; ++level) {
        LOGGER_TRACE("Process out-of-core level: " << level);

        size_t new_slice_left = 0,
               new_slice_right = 0;

        for (size_t nid = slice_left; nid <= slice_right; ++nid) {
            bvh_node& current_node = nodes_[nid];
            // make sure that current node is out-of-core
            assert(current_node.is_out_of_core());

            // split and compute child bounding boxes
            basic_algorithms::splitted_array<surfel_disk_array> surfel_arrays;

            basic_algorithms::sort_and_split(current_node.disk_array(),
                                             surfel_arrays,
                                             current_node.get_bounding_box(),
                                             current_node.get_bounding_box().get_longest_axis(),
                                             fan_factor_,
                                             memory_limit_);

            // iterate through children
            for (size_t i = 0; i < surfel_arrays.size(); ++i) {
                uint32_t child_id = get_child_id(nid, i);
                nodes_[child_id] = bvh_node(child_id, level + 1,
                                            surfel_arrays[i].second,
                                            surfel_arrays[i].first);
                if (nid == slice_left && i == 0)
                    new_slice_left = child_id;
                if (nid == slice_right && i == surfel_arrays.size() - 1)
                    new_slice_right = child_id;
            }

            current_node.reset();

            // percent counter
            ++processed_nodes;
            uint8_t new_percent_processed = (uint8_t)((float)(processed_nodes/first_leaf_ * 100));
            if (percent_processed != new_percent_processed)
            {
                percent_processed = new_percent_processed;
                //std::cout << "\r" << (uint8_t)percent_processed << "% processed" << std::flush;
            }

        }

        // expand the slice
        slice_left = new_slice_left;
        slice_right = new_slice_right;
    }

    // construct next level in-core
    for (size_t nid = slice_left; nid <= slice_right; ++nid) {
        bvh_node& current_node = nodes_[nid];

        // make sure that current node is out-of-core and switch to in-core (unless root node)
        if (nid > 0) {
            assert(current_node.is_out_of_core());
            current_node.load_from_disk();
        }
        LOGGER_TRACE("Process subbvh in-core at node " << nid);
        // process subbvh and save leafs
        downsweep_subtree_in_core(current_node, disk_leaf_destination, processed_nodes,
                                  percent_processed,
                                  leaf_level_access);
    }
    //std::cout << std::endl << std::endl;

    input_file_disk_access->close();
    state_ = state_type::after_downsweep;
}

void bvh::
downsweep_subtree_in_core( const bvh_node& node,
                           size_t& disk_leaf_destination,
                           uint32_t& processed_nodes,
                           uint8_t& percent_processed,
                           shared_file leaf_level_access)
{


    size_t slice_left = node.node_id(),
           slice_right = node.node_id();

    for (uint32_t level = node.depth(); level < depth_; ++level) {
        LOGGER_TRACE("Process in-core level " << level);

        size_t new_slice_left = 0,
               new_slice_right = 0;

        spawn_split_node_jobs(slice_left, slice_right, new_slice_left, new_slice_right, level);

        // expand the slice
        slice_left = new_slice_left;
        slice_right = new_slice_right;
    }

    LOGGER_TRACE("Compute node properties for leaves");

    spawn_compute_bounding_boxes_downsweep_jobs(slice_left, slice_right);

    LOGGER_TRACE("Save leaves to disk");
    // save leaves to disk
    for (size_t nid = slice_left; nid <= slice_right; ++nid) {
        bvh_node& current_node = nodes_[nid];
        current_node.flush_to_disk(leaf_level_access,
                                 disk_leaf_destination, true);
        disk_leaf_destination += current_node.disk_array().length();
    }
}

void bvh::compute_normal_and_radius(
    const bvh_node* source_node,
    const normal_computation_strategy& normal_computation_strategy,
    const radius_computation_strategy& radius_computation_strategy)
{
    for (size_t k = 0; k < max_surfels_per_node_; ++k)
    {
        if (k < source_node->mem_array().length())
        {
            // read surfel
            surfel surf = source_node->mem_array().read_surfel(k);

            uint16_t num_nearest_neighbours_to_search = std::max(radius_computation_strategy.number_of_neighbours(),
                                                                 normal_computation_strategy.number_of_neighbours());

            auto const& max_nearest_neighbours = get_nearest_neighbours(surfel_id_t(source_node->node_id(), k), num_nearest_neighbours_to_search);
            // compute radius
            real radius = radius_computation_strategy.compute_radius(*this, surfel_id_t(source_node->node_id(), k), max_nearest_neighbours);

            // compute normal
            vec3f normal = normal_computation_strategy.compute_normal(*this, surfel_id_t(source_node->node_id(), k), max_nearest_neighbours);            

            // write surfel
            surf.radius() = radius;
            surf.normal() = normal;
            source_node->mem_array().write_surfel(surf, k);
        }
    }
}

void bvh::get_descendant_leaves(
     const size_t node,
     std::vector<size_t>& result,
     const size_t first_leaf,
     const std::unordered_set<size_t>& excluded_leaves) const
{
    if (node < first_leaf) // inner node
    {
        for (uint16_t i = 0; i < fan_factor_; ++i)
        {
            get_descendant_leaves(get_child_id(node, i), result, first_leaf, excluded_leaves);
        }
    }
    else // leaf node
    {
        if (excluded_leaves.find(node) == excluded_leaves.end())
        {
            result.push_back(node);
        }
    }
}

void bvh::get_descendant_nodes(
     const size_t node,
     std::vector<size_t>& result,
     const size_t desired_depth,
     const std::unordered_set<size_t>& excluded_nodes) const
{
    size_t node_depth = std::log((node + 1) * (fan_factor_ - 1)) / std::log(fan_factor_);
    if (node_depth == desired_depth)
    {
        if (excluded_nodes.find(node) == excluded_nodes.end())
        {
            result.push_back(node);
        }
    }
    //node is above desired depth
    else
    {
        for (uint16_t i = 0; i < fan_factor_; ++i)
        {
            get_descendant_nodes(get_child_id(node, i), result, desired_depth, excluded_nodes);
        }
    }
}





std::vector<std::pair<surfel_id_t, real>> bvh::
get_nearest_neighbours(
    const surfel_id_t target_surfel,
    const uint32_t number_of_neighbours) const
{
    size_t current_node = target_surfel.node_idx;
    std::unordered_set<size_t> processed_nodes;
    vec3r center = nodes_[target_surfel.node_idx].mem_array().read_surfel(target_surfel.surfel_idx).pos();

    std::vector<std::pair<surfel_id_t, real>> candidates;
    real max_candidate_distance = std::numeric_limits<real>::infinity();

    // check own node
    for (size_t i = 0; i < nodes_[current_node].mem_array().length(); ++i)
    {
        if (i != target_surfel.surfel_idx)
        {
            const surfel current_surfel = nodes_[current_node].mem_array().read_surfel(i);
            real distance_to_center = scm::math::length_sqr(center - current_surfel.pos());

            if (candidates.size() < number_of_neighbours || (distance_to_center < max_candidate_distance))
            {
                if (candidates.size() == number_of_neighbours)
                    candidates.pop_back();

                candidates.push_back(std::make_pair(surfel_id_t(current_node, i), distance_to_center));

                for (uint16_t k = candidates.size() - 1; k > 0; --k)
                {
                    if (candidates[k].second < candidates[k - 1].second)
                    {
                        std::swap(candidates[k], candidates[k - 1]);
                    }
                    else
                        break;
                }

                max_candidate_distance = candidates.back().second;

            }

        }

    }

    processed_nodes.insert(current_node);

    // check rest of kd-bvh
    sphere candidates_sphere = sphere(center, sqrt(max_candidate_distance));

    while ( (!nodes_[current_node].get_bounding_box().contains(candidates_sphere)) &&
            (current_node != 0) )
    {

        current_node = get_parent_id(current_node);

        std::vector<size_t> unvisited_descendant_nodes;

        get_descendant_nodes(current_node, unvisited_descendant_nodes, nodes_[target_surfel.node_idx].depth(), processed_nodes);


        for (auto adjacent_node : unvisited_descendant_nodes)
        {

            if (candidates_sphere.intersects_or_contains(nodes_[adjacent_node].get_bounding_box()))
            {
                // assert(nodes_[adjacent_node].is_out_of_core());

                for (size_t i = 0; i < nodes_[adjacent_node].mem_array().length(); ++i)
                {
                    if (!(adjacent_node == target_surfel.node_idx && i == target_surfel.surfel_idx))
                    {
                        const surfel current_surfel = nodes_[adjacent_node].mem_array().read_surfel(i);
                        real distance_to_center = scm::math::length_sqr(center - current_surfel.pos());

                        if (candidates.size() < number_of_neighbours || (distance_to_center < max_candidate_distance))
                        {
                            if (candidates.size() == number_of_neighbours)
                                candidates.pop_back();

                            candidates.push_back(std::make_pair(surfel_id_t(adjacent_node, i), distance_to_center));

                            for (uint16_t k = candidates.size() - 1; k > 0; --k)
                            {
                                if (candidates[k].second < candidates[k - 1].second)
                                {
                                    std::swap(candidates[k], candidates[k - 1]);
                                }
                                else
                                    break;
                            }

                            max_candidate_distance = candidates.back().second;

                        }
                    }

                }

                processed_nodes.insert(adjacent_node);
                candidates_sphere = sphere(center, sqrt(max_candidate_distance));
            }

        }

    }

    return candidates;
}


std::vector<std::pair<surfel_id_t, real>> bvh::
get_nearest_neighbours_in_nodes(
    const surfel_id_t target_surfel,
    const std::vector<node_id_type>& target_nodes,
    const uint32_t number_of_neighbours) const
{
    size_t current_node = target_surfel.node_idx;
    vec3r center = nodes_[target_surfel.node_idx].mem_array().read_surfel(target_surfel.surfel_idx).pos();

    std::vector<std::pair<surfel_id_t, real>> candidates;
    real max_candidate_distance = std::numeric_limits<real>::infinity();

    // check own node
    for (size_t i = 0; i < nodes_[current_node].mem_array().length(); ++i)
    {
        if (i != target_surfel.surfel_idx)
        {
            const surfel current_surfel = nodes_[current_node].mem_array().read_surfel(i);
            real distance_to_center = scm::math::length_sqr(center - current_surfel.pos());

            if (candidates.size() < number_of_neighbours || (distance_to_center < max_candidate_distance))
            {
                if (candidates.size() == number_of_neighbours)
                    candidates.pop_back();

                candidates.push_back(std::make_pair(surfel_id_t(current_node, i), distance_to_center));

                for (uint16_t k = candidates.size() - 1; k > 0; --k)
                {
                    if (candidates[k].second < candidates[k - 1].second)
                    {
                        std::swap(candidates[k], candidates[k - 1]);
                    }
                    else
                        break;
                }

                max_candidate_distance = candidates.back().second;
            }
        }
    }

    // check remaining nodes in vector
    sphere candidates_sphere = sphere(center, sqrt(max_candidate_distance));
    for (auto adjacent_node: target_nodes)
    {
        if (adjacent_node != current_node)
        {
            if (candidates_sphere.intersects_or_contains(nodes_[adjacent_node].get_bounding_box()))
            {
                // assert(nodes_[adjacent_node].is_out_of_core());

                for (size_t i = 0; i < nodes_[adjacent_node].mem_array().length(); ++i)
                {
                    if (!(adjacent_node == target_surfel.node_idx && i == target_surfel.surfel_idx))
                    {
                        const surfel current_surfel = nodes_[adjacent_node].mem_array().read_surfel(i);
                        real distance_to_center = scm::math::length_sqr(center - current_surfel.pos());

                        if (candidates.size() < number_of_neighbours || (distance_to_center < max_candidate_distance))
                        {
                            if (candidates.size() == number_of_neighbours)
                                candidates.pop_back();

                            candidates.push_back(std::make_pair(surfel_id_t(adjacent_node, i), distance_to_center));

                            for (uint16_t k = candidates.size() - 1; k > 0; --k)
                            {
                                if (candidates[k].second < candidates[k - 1].second)
                                {
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

            candidates_sphere = sphere(center, sqrt(max_candidate_distance));
        }
    }
    return candidates;
}

std::vector<std::pair<surfel_id_t, real> > bvh::
extract_approximate_natural_neighbours(vec3r const& point_of_interest, std::vector< std::pair<surfel, real> > const& nearest_neighbours ) const {

    std::vector<std::pair<surfel_id_t, real>> natural_neighbour_ids;

    uint32_t num_nearest_neighbours = nearest_neighbours.size();
    //compile points
    double* points = new double[num_nearest_neighbours * 3];

    uint32_t point_counter = 0;

    std::vector<vec3r> nn_positions;

    nn_positions.reserve(num_nearest_neighbours);

    for (auto const& near_neighbour : nearest_neighbours) {

        auto const neighbour_pos = near_neighbour.first.pos();
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
    double* coords = new double[2*num_nearest_neighbours];
    scm::math::vec3f plane_right = plane.get_right();
    for (uint32_t i = 0; i < num_nearest_neighbours; ++i) {
        vec3r v = vec3r(points[3*i+0], points[3*i+1], points[3*i+2]);
        vec2r c = plane_t::project(plane, plane_right, v);
        coords[2*i+0] = c[0];
        coords[2*i+1] = c[1];

        if (c[0] != c[0] || c[1] != c[1]) { //is nan?
            is_projection_valid = false;
        }
    }
    
    if (!is_projection_valid) {
        delete[] points;
        delete[] coords;

        return natural_neighbour_ids;
    }


    //cgal delaunay triangluation
    Dh2 delaunay_triangulation;

    std::vector<scm::math::vec2f> neighbour_2d_coord_pairs;

    for (uint32_t i = 0; i < num_nearest_neighbours; ++i) {
        nni_sample_t sp;
        sp.xy_ = scm::math::vec2f(coords[2*i+0], coords[2*i+1]);
        Point2 p(sp.xy_.x, sp.xy_.y);
        delaunay_triangulation.insert(p);
        neighbour_2d_coord_pairs.emplace_back(coords[2*i+0], coords[2*i+1]);
    }
    
    vec2r coord_poi = plane_t::project(plane, plane_right, point_of_interest);

    Point2 poi_2d(coord_poi.x, coord_poi.y);
    
    std::vector<std::pair<K::Point_2, K::FT>> sibson_coords;
    CGAL::Triple<std::back_insert_iterator<std::vector<std::pair<K::Point_2, K::FT>>>, K::FT, bool> result = 
        natural_neighbor_coordinates_2(
            delaunay_triangulation,
            poi_2d,
            std::back_inserter(sibson_coords));


    //CGAL::sibson_natural_neighbor_coordinates_2();

    if (!result.third) {
        delete[] points;
        delete[] coords;
        return natural_neighbour_ids;
    }

    std::vector<std::pair<uint32_t, double> > nni_weights;

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
        surfel_id_t natural_neighbour_id_pair = surfel_id_t(it.first, it.first);

        natural_neighbour_ids.emplace_back(natural_neighbour_id_pair, it.second);

    }

    delete[] points;
    delete[] coords;
    
    sibson_coords.clear();
    nni_weights.clear();



    return natural_neighbour_ids;


}

std::vector<std::pair<surfel_id_t, real>> bvh::
get_natural_neighbours(
    const surfel_id_t& target_surfel,
    const bool search_for_neighbours,
    std::vector<std::pair<surfel_id_t, real>> const& nearest_neighbours) const {

    std::vector<std::pair<surfel_id_t, real>> nearest_neighbour_ids;

    if(search_for_neighbours) {
        nearest_neighbour_ids = get_nearest_neighbours(target_surfel, 24);
    } else {
        for(int32_t k = 0; k < 24; ++k) {
            nearest_neighbour_ids.emplace_back(nearest_neighbours[k]);
        }
    }
    //get_nearest_neighbours(target_surfel, num_nearest_neighbours);

    std::random_shuffle(nearest_neighbour_ids.begin(), nearest_neighbour_ids.end());
    
    auto const& bvh_nodes = nodes_;

    std::vector< std::pair<surfel, real>> nearest_neighbour_copies;
    for (auto const& idx_pair : nearest_neighbour_ids) {

        surfel const current_nearest_neighbour_surfel = 
            bvh_nodes[idx_pair.first.node_idx].mem_array().read_surfel(idx_pair.first.surfel_idx);

        nearest_neighbour_copies.push_back( std::make_pair(current_nearest_neighbour_surfel,0.0) );
    }
    
    //project point of interest
    vec3r point_of_interest = nodes_[target_surfel.node_idx].mem_array().read_surfel(target_surfel.surfel_idx).pos();

    std::vector<std::pair<surfel_id_t, real>> natural_neighbour_with_local_nearest_neighbour_ids = 
        extract_approximate_natural_neighbours(point_of_interest, nearest_neighbour_copies);

    for (auto& nn_entry : natural_neighbour_with_local_nearest_neighbour_ids ) {
        nn_entry.first = nearest_neighbour_ids[nn_entry.first.surfel_idx].first;
    }

    return natural_neighbour_with_local_nearest_neighbour_ids;
}

std::vector<std::pair<surfel, real> > bvh::
get_locally_natural_neighbours(std::vector<surfel> const& potential_neighbour_vec,
                               vec3r const& poi,
                               uint32_t num_nearest_neighbours) const {

    num_nearest_neighbours = std::max(uint32_t(3), num_nearest_neighbours);

    std::vector< std::pair<surfel,real>> k_nearest_neighbours;
    
    for (auto const& neigh : potential_neighbour_vec) {
        double length_squared = scm::math::length_sqr(neigh.pos() - poi);

        bool push_surfel = false;
        if ( k_nearest_neighbours.size() < num_nearest_neighbours ) {
            push_surfel = true;
        } else if ( length_squared < k_nearest_neighbours.back().second ) {
            k_nearest_neighbours.pop_back();
            push_surfel = true;
        }

        if(push_surfel) {
            k_nearest_neighbours.push_back(std::make_pair(neigh, length_squared) );

            for (uint16_t k = k_nearest_neighbours.size() - 1; k > 0; --k)
            {
                if (k_nearest_neighbours[k].second < k_nearest_neighbours[k - 1].second)
                {
                    std::swap(k_nearest_neighbours[k], k_nearest_neighbours[k - 1]);
                }
                else
                    break;
            }
        }
    }

    std::vector< std::pair<surfel_id_t, real>> local_nn_id_weight_pairs = 
        extract_approximate_natural_neighbours(poi, k_nearest_neighbours);

    std::vector< std::pair<surfel, real> > nni_weight_pairs;

    for (auto const& entry : local_nn_id_weight_pairs) {
        nni_weight_pairs.push_back(std::make_pair(k_nearest_neighbours[entry.first.surfel_idx].first, entry.second));
    }

    return nni_weight_pairs;
}

void bvh::
spawn_create_lod_jobs(const uint32_t first_node_of_level, 
                      const uint32_t last_node_of_level,
                      const reduction_strategy& reduction_strgy,
                      bool resample) {
    uint32_t const num_threads = std::thread::hardware_concurrency();

    working_queue_head_counter_.initialize(first_node_of_level); //let the threads fetch a node idx
    std::vector<std::thread> threads;

    for(uint32_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
        bool update_percentage = (0 == thread_idx);
        threads.push_back(std::thread(&bvh::thread_create_lod, this, 
                                      first_node_of_level, last_node_of_level, 
                                      update_percentage, 
                                      std::cref(reduction_strgy),
                                      resample  ) );
    }

    for(auto& thread : threads){
        thread.join();
    }
}

void bvh::
spawn_compute_attribute_jobs(const uint32_t first_node_of_level, 
                             const uint32_t last_node_of_level,
                             const normal_computation_strategy& normal_strategy, 
                             const radius_computation_strategy& radius_strategy) {
    uint32_t const num_threads = std::thread::hardware_concurrency();
    working_queue_head_counter_.initialize(first_node_of_level); //let the threads fetch a node idx
    std::vector<std::thread> threads;

    for(uint32_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
        bool update_percentage = (0 == thread_idx);
        threads.push_back(std::thread(&bvh::thread_compute_attributes, this, 
                                      first_node_of_level, last_node_of_level, 
                                      update_percentage, 
                                      std::cref(normal_strategy), std::cref(radius_strategy)  ) );
    }

    for(auto& thread : threads){
        thread.join();
    }
}

void bvh::
spawn_compute_bounding_boxes_downsweep_jobs(const uint32_t slice_left, 
                                            const uint32_t slice_right) {
    uint32_t const num_threads = std::thread::hardware_concurrency();
    working_queue_head_counter_.initialize(0); //let the threads fetch a local thread idx
    std::vector<std::thread> threads;

    for(uint32_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
        bool update_percentage = (0 == thread_idx);
        threads.push_back(std::thread(&bvh::thread_compute_bounding_boxes_downsweep, this, 
                                      slice_left, slice_right, 
                                      update_percentage,
                                      num_threads) );
    }

    for(auto& thread : threads){
        thread.join();
    }
}

void bvh::
resample(surfel_mem_array const&  joined_input, surfel_mem_array& output_mem_array, 
          real const avg_radius) const
{
    
    for(uint32_t i = 0; i < joined_input.mem_data()->size(); ++i){
        output_mem_array.mem_data()->push_back( joined_input.read_surfel(i) );
    }

    const real max_radius_difference = 0.0; //
    //real avg_radius = node.avg_surfel_radius();

    auto compute_new_position = [] (surfel const& plane_ref_surfel, real radius_offset, real rot_angle) {
        vec3r new_position (0.0, 0.0, 0.0);

        vec3f n = plane_ref_surfel.normal();

        //from random_point_on_surfel() in surfe.cpp
        //find a vector orthogonal to given normal vector
        scm::math::vec3f  u(std::numeric_limits<float>::lowest(),
                            std::numeric_limits<float>::lowest(),
                            std::numeric_limits<float>::lowest());
        if(n.z != 0.0) {
            u = scm::math::vec3f( 1, 1, (-n.x - n.y) / n.z);
        } else if (n.y != 0.0) {
            u = scm::math::vec3f( 1, (-n.x - n.z) / n.y, 1);
        } else {
            u = scm::math::vec3f( (-n.y - n.z)/n.x, 1, 1);
        }
        scm::math::normalize(u);
        vec3f p = scm::math::normalize(scm::math::cross(n,u)); //plane of rotation given by cross product of n and u
        u = p; //^^

        //vector rotation according to: https://en.wikipedia.org/wiki/Rodrigues'_rotation_formula
        //rotation around the normal vector n
        vec3f u_rotated = u*cos(rot_angle) + scm::math::normalize(scm::math::cross(u,n))*sin(rot_angle) + n*scm::math::dot(u,n)*(1-cos(rot_angle));

        //extend vector  lenght to match desired radius 
        u_rotated = scm::math::normalize(u_rotated)*radius_offset;
        
        new_position = plane_ref_surfel.pos() + u_rotated;
        return new_position; 
    };

    //create new vector to store node surfels; unmodified + modified ones
    surfel_mem_array modified_mem_array (std::make_shared<surfel_vector>(surfel_vector()), 0, 0);


    for(uint32_t i = 0; i < output_mem_array.mem_data()->size(); ++i){

        surfel current_surfel = output_mem_array.read_surfel(i);

        //replace big surfels by collection of average-size surfels
        if (current_surfel.radius() - avg_radius > max_radius_difference){
        //if(false) {     
            //how many times does average radius fit into big radius
            int iteration_level = floor(current_surfel.radius()/(2*avg_radius));             

            //keep all surfel properties but shrink its radius to the average radius
            surfel new_surfel = current_surfel;
            new_surfel.radius() = avg_radius;
            //new_surfel.color() = vec3b(100, 120, 0); //^^change color for test reasons
            output_mem_array.write_surfel(new_surfel, i);            
            

            //create new average-size surfels to fill up the area orininally covered by bigger surfel            
            for(int k = 1; k <= (iteration_level - 1); ++k){
               uint16_t num_new_surfels = 6*k; //^^ formula basis https://en.wikipedia.org/wiki/Circle_packing_in_a_circle
               real angle_offset = (360.0) / num_new_surfels;
               real angle = 0.0; //reset 
               for(int j = 0; j < num_new_surfels; ++j){
                    real radius_offset = k*2*avg_radius; 
                    new_surfel.pos() = compute_new_position(current_surfel, radius_offset, angle);
                    modified_mem_array.mem_data()->push_back(new_surfel);
                    angle = angle + angle_offset;                    
               }
            }          
        }
    }

    for(uint32_t i = 0; i < modified_mem_array.mem_data()->size(); ++i) {
        output_mem_array.mem_data()->push_back( modified_mem_array.mem_data()->at(i) );
    }

}

void bvh::
spawn_compute_bounding_boxes_upsweep_jobs(const uint32_t first_node_of_level, 
                                          const uint32_t last_node_of_level,
                                          const int32_t level) {
    uint32_t const num_threads = std::thread::hardware_concurrency();
    working_queue_head_counter_.initialize(0); //let the threads fetch a local thread idx
    std::vector<std::thread> threads;

    for(uint32_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
        bool update_percentage = (0 == thread_idx);
        threads.push_back(std::thread(&bvh::thread_compute_bounding_boxes_upsweep, this, 
                                      first_node_of_level, last_node_of_level, 
                                      update_percentage,
                                      level, num_threads) );
    }

    for(auto& thread : threads){
        thread.join();
    }
}

void bvh::
spawn_split_node_jobs(size_t& slice_left,
                      size_t& slice_right,
                      size_t& new_slice_left,
                      size_t& new_slice_right,
                      const uint32_t level) {
    uint32_t const num_threads = std::thread::hardware_concurrency();
    working_queue_head_counter_.initialize(0); //let the threads fetch a local thread idx
    std::vector<std::thread> threads;

    for(uint32_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
        bool update_percentage = (0 == thread_idx);
        threads.push_back(std::thread(&bvh::thread_split_node_jobs, this, 
                                      std::ref(slice_left), std::ref(slice_right), 
                                      std::ref(new_slice_left), std::ref(new_slice_right),
                                      update_percentage,
                                      level, num_threads) );
    }

    for(auto& thread : threads){
        thread.join();
    }
}

void bvh::
thread_create_lod(const uint32_t start_marker,
                  const uint32_t end_marker,
                  const bool update_percentage,
                  const reduction_strategy& reduction_strgy,
                  bool do_resample) {
    uint32_t node_index = working_queue_head_counter_.increment_head();
    
    while(node_index < end_marker) {
        bvh_node* current_node = &nodes_.at(node_index);
        std::vector<real> avg_radius_vector;
        // If a node has no data yet, calculate it based on child nodes.
        if (!current_node->is_in_core() && !current_node->is_out_of_core()) {

            std::vector<surfel_mem_array*> child_mem_arrays;
            std::vector<surfel_mem_array*> subsampled_child_mem_arrays;
            for (uint8_t child_index = 0; child_index < fan_factor_; ++child_index) {
                size_t child_id = this->get_child_id(current_node->node_id(), child_index);
                bvh_node* child_node = &nodes_.at(child_id);

                child_mem_arrays.push_back(&child_node->mem_array());
                avg_radius_vector.push_back(child_node->avg_surfel_radius());
            }
            
            real reduction_error;
            
            //simplified data will be stored here
            surfel_mem_array reduction (std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

            if (!do_resample){

            reduction = reduction_strgy.create_lod(reduction_error, 
                                                   child_mem_arrays, max_surfels_per_node_, 
                                                   (*this), get_child_id(current_node->node_id(), 0) );

            }
            else{
                std::vector<surfel_mem_array> mem_array_obj;
                for (unsigned i=0; i<child_mem_arrays.size(); ++i){
                mem_array_obj.emplace_back( std::make_shared<surfel_vector>(surfel_vector()), 0, 0 );
                }


                subsampled_child_mem_arrays.reserve(child_mem_arrays.size());
                for (unsigned i=0; i<child_mem_arrays.size(); ++i){
                    real current_avg_radius = avg_radius_vector.at(i);
                    surfel_mem_array current_child_node = *child_mem_arrays.at(i);
                    resample(current_child_node,mem_array_obj[i], current_avg_radius);
                    mem_array_obj[i].set_length(mem_array_obj[i].mem_data()->size());
                    subsampled_child_mem_arrays.push_back(&mem_array_obj.at(i));
                }


                reduction = reduction_strgy.create_lod(reduction_error, 
                                                       subsampled_child_mem_arrays, max_surfels_per_node_, 
                                                       (*this), get_child_id(current_node->node_id(), 0) );

            }
            


            current_node->reset(reduction);
            current_node->set_reduction_error(reduction_error);

        }

        node_index = working_queue_head_counter_.increment_head();
    }
}

void bvh::
thread_compute_attributes(const uint32_t start_marker,
                          const uint32_t end_marker,
                          const bool update_percentage,
                          const normal_computation_strategy& normal_strategy, 
                          const radius_computation_strategy& radius_strategy) {

    uint32_t node_index = working_queue_head_counter_.increment_head();
    
    uint16_t percentage = 0;
    uint32_t length_of_level = (end_marker-start_marker) + 1;

    while(node_index < end_marker) {
        bvh_node* current_node = &nodes_.at(node_index);


        // Calculate and set node properties.
        compute_normal_and_radius(current_node, normal_strategy, radius_strategy);

        if(update_percentage) {
            uint16_t new_percentage = int32_t(float(node_index-start_marker)/(length_of_level) * 100);
            if (percentage < new_percentage)
            {
                percentage = new_percentage;
                std::cout << "\r" << percentage << "% processed" << std::flush;
            }
        }
        node_index = working_queue_head_counter_.increment_head();
    }
};


void bvh::
thread_compute_bounding_boxes_downsweep(const uint32_t slice_left,
                                        const uint32_t slice_right,
                                        const bool update_percentage,
                                        const uint32_t num_threads) {

    uint32_t thread_idx = working_queue_head_counter_.increment_head();

    uint32_t total_num_slices= (slice_right-slice_left) + 2;
    uint32_t num_slices_per_thread = std::ceil(float(total_num_slices) / num_threads);

    uint32_t local_start_index = slice_left + thread_idx * num_slices_per_thread;
    uint32_t local_end_index   = slice_left + (thread_idx + 1) * num_slices_per_thread;

    for(uint32_t slice_index = local_start_index; num_slices_per_thread < local_end_index; ++slice_index) {
        //early termination if number of nodes could not be evenly divided
        if(slice_index > slice_right) {
            break;
        }

        bvh_node& current_node = nodes_[slice_index];
        auto props = basic_algorithms::compute_properties(current_node.mem_array(), rep_radius_algo_, false);
        current_node.set_avg_surfel_radius(props.rep_radius);
        current_node.set_centroid(props.centroid);
        current_node.set_bounding_box(props.bbox);
    }
}

void bvh::
thread_compute_bounding_boxes_upsweep(const uint32_t start_marker,
                                      const uint32_t end_marker,
                                      const bool update_percentage,
                                      const int32_t level,
                                      const uint32_t num_threads) {

    uint32_t thread_idx = working_queue_head_counter_.increment_head();

    uint32_t total_num_nodes = (end_marker-start_marker) + 1;
    uint32_t num_nodes_per_thread = std::ceil(float(total_num_nodes) / num_threads);

    uint32_t local_start_index = start_marker + thread_idx * num_nodes_per_thread;
    uint32_t local_end_index   = start_marker + (thread_idx + 1) * num_nodes_per_thread;

    for(uint32_t node_index = local_start_index; node_index < local_end_index; ++node_index) {
        //early termination if number of nodes could not be evenly divided
        if(node_index >= end_marker) {
            break;
        }

        bvh_node* current_node = &nodes_.at(node_index);

        basic_algorithms::surfel_group_properties props = basic_algorithms::compute_properties(current_node->mem_array(), 
                                                                                                rep_radius_algo_);

        bounding_box node_bounding_box;
        node_bounding_box.expand(props.bbox);

        if (level < int32_t(depth_) ) {
            for (int32_t child_index = 0; child_index < fan_factor_; ++child_index) {
                uint32_t child_id = this->get_child_id(current_node->node_id(), child_index);
                bvh_node* child_node = &nodes_.at(child_id);

                node_bounding_box.expand(child_node->get_bounding_box());
            }
        }

        current_node->set_avg_surfel_radius(props.rep_radius);
        current_node->set_centroid(props.centroid);

        current_node->set_bounding_box(node_bounding_box);
        current_node->calculate_statistics();
    }
}

void bvh::
thread_remove_outlier_jobs(const uint32_t start_marker,
                           const uint32_t end_marker,
                           const uint32_t num_outliers,
                           const uint16_t num_neighbours,
                           std::vector< std::pair<surfel_id_t, real> >&  intermediate_outliers_for_thread) {

    uint32_t node_idx = working_queue_head_counter_.increment_head();

    while(node_idx < end_marker ) {

        bvh_node* current_node = &nodes_.at(node_idx);
        
        for( size_t surfel_idx = 0; surfel_idx < current_node->mem_array().length(); ++surfel_idx) {

            std::vector<std::pair<surfel_id_t, real>> const nearest_neighbour_vector 
                = get_nearest_neighbours(surfel_id_t(node_idx, surfel_idx), num_neighbours);

            double avg_dist = 0.0;

            if( nearest_neighbour_vector.size() ) {
                for( auto const& nearest_neighbour_pair : nearest_neighbour_vector ) {
                    avg_dist += nearest_neighbour_pair.second;
                }

                avg_dist /= nearest_neighbour_vector.size();
            }

            bool insert_element = false;
            if( intermediate_outliers_for_thread.size() < num_outliers ) {
                insert_element = true;

            } else if ( avg_dist > intermediate_outliers_for_thread.back().second ) {
                intermediate_outliers_for_thread.pop_back();
                insert_element = true;
            }

            if( insert_element ) {
                intermediate_outliers_for_thread.push_back( std::make_pair(surfel_id_t(node_idx, surfel_idx), avg_dist) );

                for (uint32_t k = intermediate_outliers_for_thread.size() - 1; k > 0; --k)
                {
                    if (intermediate_outliers_for_thread[k].second > intermediate_outliers_for_thread[k - 1].second)
                    {
                        std::swap(intermediate_outliers_for_thread[k], intermediate_outliers_for_thread[k - 1]);
                    }
                    else
                        break;
                }             
            }
        }

        node_idx = working_queue_head_counter_.increment_head();
    }

}

void bvh::
thread_split_node_jobs(size_t& slice_left,
                       size_t& slice_right,
                       size_t& new_slice_left,
                       size_t& new_slice_right,
                       const bool update_percentage,
                       const int32_t level,
                       const uint32_t num_threads) {

    const uint32_t sort_parallelizm_thres = 2;

    uint32_t thread_idx = working_queue_head_counter_.increment_head();

    uint32_t total_num_slices = (slice_right-slice_left) + 2;
    uint32_t num_slices_per_thread = std::ceil(float(total_num_slices) / num_threads);

    uint32_t local_start_index = slice_left + thread_idx * num_slices_per_thread;
    uint32_t local_end_index   = slice_left + (thread_idx + 1) * num_slices_per_thread;

    for(uint32_t slice_index = local_start_index; slice_index < local_end_index; ++slice_index) {

        //early termination if number of nodes could not be evenly divided
        if(slice_index > slice_right) {
            break;
        }

        bvh_node& current_node = nodes_[slice_index];
        // make sure that current node is in-core
        assert(current_node.is_in_core());

        // split and compute child bounding boxes
        basic_algorithms::splitted_array<surfel_mem_array> surfel_arrays;
        basic_algorithms::sort_and_split(current_node.mem_array(),
                                        surfel_arrays,
                                        current_node.get_bounding_box(),
                                        current_node.get_bounding_box().get_longest_axis(),
                                        fan_factor_,
                                        (slice_right - slice_left) < sort_parallelizm_thres);

        // iterate through children
        for (size_t i = 0; i < surfel_arrays.size(); ++i) {
            uint32_t child_id = get_child_id(slice_index, i);
            nodes_[child_id] = bvh_node(child_id, level + 1,
                                       surfel_arrays[i].second,
                                       surfel_arrays[i].first);
            if (slice_index == slice_left && i == 0)
                new_slice_left = child_id;
            if (slice_index == slice_right && i == surfel_arrays.size() - 1)
                new_slice_right = child_id;
        }

        current_node.reset();

    }
}



void bvh::
upsweep(const reduction_strategy& reduction_strgy, 
        const normal_computation_strategy& normal_strategy, 
        const radius_computation_strategy& radius_strategy,
        bool recompute_leaf_level,
        bool resample) {
    // Start at bottom level and move up towards root.
    for (int32_t level = depth_; level >= 0; --level)
    {
        std::cout << "Entering level: " << level << std::endl;
    
        uint32_t first_node_of_level = get_first_node_id_of_depth(level);
        uint32_t last_node_of_level = get_first_node_id_of_depth(level) + get_length_of_depth(level);

        // Loading is not thread-safe, so load everything before starting parallel operations.
        for (uint32_t node_index = first_node_of_level; node_index < last_node_of_level; ++node_index)
        {
            bvh_node* current_node = &nodes_.at(node_index);

            if (current_node->is_out_of_core())
            {
                current_node->load_from_disk();
            }
        }
        // Iterate over nodes of current tree level.
        // First apply reduction strategy, since calculation of attributes might depend on surfel data of nodes in same level.
        if(level != int32_t(depth_) ) {
            spawn_create_lod_jobs(first_node_of_level, last_node_of_level, reduction_strgy, resample);
        }

        {
            // skip the leaf level attribute computation if it was not requested or necessary
            if( !(level == int32_t(depth_) && !recompute_leaf_level ) ) {
                spawn_compute_attribute_jobs(first_node_of_level, last_node_of_level, normal_strategy, radius_strategy);
            }

            spawn_compute_bounding_boxes_upsweep_jobs(first_node_of_level, last_node_of_level, level);

            std::cout << std::endl;
        }

        //if(level < 3){
              //std::fstream txtfile ("node_stats.txt");
        real mean_radius_sd = 0.0;
        uint counter = 1;
        for(uint32_t node_index = first_node_of_level; node_index < last_node_of_level; ++node_index){

            bvh_node* current_node = &nodes_.at(node_index);

            //std::cout<< "mean radius: "<< (*current_node).node_stats().mean_radius() << std::endl;
            mean_radius_sd = mean_radius_sd + (*current_node).node_stats().radius_sd();
            counter++;
              /*if (txtfile.is_open())
              {
                txtfile << "Entering level: " << level << std::endl;
                txtfile <<"mean radius: "<< (*current_node).node_stats().mean_radius() << std::endl;
                txtfile.close();
              }
              else std::cout << "Unable to open file \n";*/
              //std::cout<< "mean radius pro node: "<< (*current_node).node_stats().mean_radius() << "\n";
        }
        mean_radius_sd = mean_radius_sd/counter;
        std::cout<< "average radius deviation pro level: "<< mean_radius_sd << "\n";

        //}
    }

    // Create level temp files
    std::vector<shared_file> level_temp_files;
    for (uint32_t level = 0; level <= depth_; ++level)
    {
        level_temp_files.push_back(std::make_shared<file>());
        std::string ext = ".lv" + std::to_string(level);
        level_temp_files.back()->open(add_to_path(base_path_, ext).string(), level != depth_);
    }

    // Save node data in proper order (from zero to last).
    for (uint32_t node_index = 0; node_index < nodes_.size(); ++node_index)
    {
        bvh_node* current_node = &nodes_.at(node_index);

        // compute node offset in file
        int32_t nid = current_node->node_id();
        for (uint32_t level = 0; level < current_node->depth(); ++level)
            nid -= uint32_t(pow(fan_factor_, level));
        nid = std::max(0, nid);

        // save computed node to disk
        current_node->flush_to_disk(level_temp_files[current_node->depth()], size_t(nid) * max_surfels_per_node_, false);
    }
    
    state_ = state_type::after_upsweep;
}


surfel_vector bvh::
remove_outliers_statistically(uint32_t num_outliers, uint16_t num_neighbours) {

    std::vector<std::vector< std::pair<surfel_id_t, real> > > intermediate_outliers;

    uint32_t const num_threads = std::thread::hardware_concurrency();
    intermediate_outliers.resize(num_threads);
    //already_resized.resize(omp_get_max_threads())


    for(uint32_t node_idx = first_leaf_; node_idx < nodes_.size(); ++node_idx) {
        bvh_node* current_node = &nodes_.at(node_idx);
        
        if (current_node->is_out_of_core())
        {
            current_node->load_from_disk();
        }
    }


    working_queue_head_counter_.initialize(first_leaf_);
    std::vector<std::thread> threads;

    for(uint32_t thread_idx = 0; thread_idx < num_threads; ++thread_idx) {
        threads.push_back(std::thread(&bvh::thread_remove_outlier_jobs, this, 
                                      first_leaf_, nodes_.size(),
                                      num_outliers, num_neighbours,
                                      std::ref(intermediate_outliers[thread_idx]) ) );
    }

    for(auto& thread : threads){
        thread.join();
    }

    std::vector< std::pair<surfel_id_t, real> >  final_outliers;

    for (auto const& ve : intermediate_outliers) {

        for(auto const& element : ve) {
            bool insert_element = false;
            if( final_outliers.size() < num_outliers ) {
                insert_element = true;
            } else if( element.second > final_outliers.back().second ) {
                final_outliers.pop_back();
                insert_element = true;
            }

            if( insert_element ) {
                final_outliers.push_back(element);

                for (uint32_t k = final_outliers.size() - 1; k > 0; --k)
                {
                    if (final_outliers[k].second > final_outliers[k - 1].second)
                    {
                        std::swap(final_outliers[k], final_outliers[k - 1]);
                    }
                    else
                        break;
                }

            }
        }
    }

    intermediate_outliers.clear();

    std::set<surfel_id_t> outlier_ids;
    for(auto const& el : final_outliers) {
        outlier_ids.insert(el.first);
    }

    surfel_vector cleaned_surfels;


    for(uint32_t node_idx = first_leaf_; node_idx < nodes_.size(); ++node_idx) {


        bvh_node* current_node = &nodes_.at(node_idx);
        
        if (current_node->is_out_of_core())
        {
            current_node->load_from_disk();
        } 

        for( uint32_t surfel_idx = 0; surfel_idx < current_node->mem_array().length(); ++surfel_idx) {

            if( outlier_ids.end() == outlier_ids.find( surfel_id_t(node_idx, surfel_idx) ) ) {
                cleaned_surfels.push_back(current_node->mem_array().read_surfel(surfel_idx) );

            }
        }

    }

    return cleaned_surfels;
}

void bvh::
serialize_tree_to_file(const std::string& output_file,
                    bool write_intermediate_data)
{
    LOGGER_TRACE("Serialize bvh to file: \"" << output_file << "\"");

    if(! write_intermediate_data) {
        assert(state_type::after_upsweep == state_);
        state_ = state_type::serialized;
    }


    bvh_stream bvh_strm;
    bvh_strm.write_bvh(output_file, *this, write_intermediate_data);
}


void bvh::
serialize_surfels_to_file(const std::string& output_file, const size_t buffer_size) const
{
    LOGGER_TRACE("Serialize surfels to file: \"" << output_file << "\"");
    node_serializer serializer(max_surfels_per_node_, buffer_size);
    serializer.open(output_file);
    serializer.serialize_nodes(nodes_);
}

void bvh::
reset_nodes()
{
    for (auto& n: nodes_) {
        if (n.is_out_of_core() && n.disk_array().file().use_count() == 1) {
            n.disk_array().file()->close(true);
        }
        n.reset();
    }
}

std::string bvh::
state_to_string(state_type state)
{
    std::map<state_type, std::string> state_typeMap = {
        {state_type::null,             "Null"},
        {state_type::empty,            "empty"},
        {state_type::after_downsweep,  "after_downsweep"},
        {state_type::after_upsweep,    "after_upsweep"},
        {state_type::serialized,       "serialized"}
    };
    return state_typeMap[state];
}

} // namespace pre
} // namespace lamure
