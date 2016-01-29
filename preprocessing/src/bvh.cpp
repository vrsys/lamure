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
#include <lamure/pre/reduction_strategy.h>
#include <lamure/pre/serialized_surfel.h>
#include <lamure/pre/plane.h>
#include <lamure/utils.h>
#include <lamure/sphere.h>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
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
    LOGGER_INFO("Fan-out factor: " << int(fan_factor_));
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
            uint8_t new_percent_processed = (int)((float)(processed_nodes/first_leaf_ * 100));
            if (percent_processed != new_percent_processed)
            {
                percent_processed = new_percent_processed;
                //std::cout << "\r" << (int)percent_processed << "% processed" << std::flush;
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
    const size_t sort_parallelizm_thres = 2;

    size_t slice_left = node.node_id(),
           slice_right = node.node_id();

    for (uint32_t level = node.depth(); level < depth_; ++level) {
        LOGGER_TRACE("Process in-core level " << level);

        size_t new_slice_left = 0,
               new_slice_right = 0;

        #pragma omp parallel for
        for (size_t nid = slice_left; nid <= slice_right; ++nid) {
            bvh_node& current_node = nodes_[nid];
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
            uint8_t new_percent_processed = (int)((float)(processed_nodes/(first_leaf_) * 100));
            if (percent_processed != new_percent_processed)
            {
                percent_processed = new_percent_processed;
                //std::cout << "\r" << (int)percent_processed << "% processed" << std::flush;
            }
        }

        // expand the slice
        slice_left = new_slice_left;
        slice_right = new_slice_right;
    }

    LOGGER_TRACE("Compute node properties for leaves");
    // compute avg surfel radius for leaves
    #pragma omp parallel for
    for (size_t nid = slice_left; nid <= slice_right; ++nid) {
        bvh_node& current_node = nodes_[nid];
        auto props = basic_algorithms::compute_properties(current_node.mem_array(), rep_radius_algo_, false);
        current_node.set_avg_surfel_radius(props.rep_radius);
        current_node.set_centroid(props.centroid);
        current_node.set_bounding_box(props.bbox);
    }

    LOGGER_TRACE("Save leaves to disk");
    // save leaves to disk
    for (size_t nid = slice_left; nid <= slice_right; ++nid) {
        bvh_node& current_node = nodes_[nid];
        current_node.flush_to_disk(leaf_level_access,
                                 disk_leaf_destination, true);
        disk_leaf_destination += current_node.disk_array().length();
    }
}

void bvh::compute_normals_and_radii(const uint16_t number_of_neighbours)
{
    size_t number_of_leaves = std::pow(fan_factor_, depth_);

    // load from disk
    for (size_t i = first_leaf_; i < nodes_.size(); ++i)
    {
        nodes_[i].load_from_disk();
    }

    size_t counter = 0;
    uint16_t percentage = 0;

    size_t old_i = first_leaf_;
    real average_radius = 0.0;
    real average_radius_for_node = 0.0;

    std::cout << std::endl << "compute normals and radii" << std::endl;

    // compute normals and radii
    #pragma omp parallel for firstprivate(old_i, average_radius_for_node) shared(average_radius) collapse(2)
    for (size_t i = first_leaf_; i < nodes_.size(); ++i)
    {
        for (size_t k = 0; k < max_surfels_per_node_; ++k)
        {

            ++counter;
            uint16_t new_percentage = int(float(counter)/(number_of_leaves*max_surfels_per_node_) * 100);
            if (percentage + 1 == new_percentage)
            {
                percentage = new_percentage;
                std::cout << "\r" << percentage << "% processed" << std::flush;
            }

            if (k < nodes_[i].mem_array().length())
            {

                // find nearest neighbours
                std::vector<std::pair<surfel_id_t, real>> neighbours = get_nearest_neighbours(surfel_id_t(i, k), number_of_neighbours);
                surfel surf = nodes_[i].mem_array().read_surfel(k);

                // compute radius
                real avg_distance = 0.0;

                for (auto const& neighbour : neighbours)
                {
                    avg_distance += sqrt(neighbour.second);
                }

                avg_distance /= neighbours.size();

                // compute normal
                // see: http://missingbytes.blogspot.com/2012/06/fitting-plane-to-point-cloud.html

                vec3r center = surf.pos();
                vec3f normal(1.0, 0.0 , 0.0);

                real sum_x_x = 0.0, sum_x_y = 0.0, sum_x_z = 0.0;
                real sum_y_y = 0.0, sum_y_z = 0.0;
                real sum_z_z = 0.0;

                for (auto const& neighbour : neighbours)
                {
                    vec3r neighbour_pos = nodes_[neighbour.first.node_idx].mem_array().read_surfel(neighbour.first.surfel_idx).pos();
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

                if (!(scm::math::determinant(matrix) == 0.0f))
                {
                    matrix = scm::math::inverse(matrix);

                    float scale = fabs(matrix[0]);
                    for (uint16_t i = 1; i < 9; ++i)
                    {
                        scale = std::max(scale, float(fabs(matrix[i])));
                    }

                    scm::math::mat3f matrix_c = matrix * (1.0f/scale);

                    vec3f vector(1.0f, 1.0f, 1.0f);
                    vec3f last_vector = vector;

                    for (uint16_t i = 0; i < 100; i++)
                    {
                        vector = (matrix_c * vector);
                        vector = scm::math::normalize(vector);

                        if (pow(scm::math::length(vector - last_vector), 2) < 1e-16f)
                        {
                            break;
                        }
                        last_vector = vector;
                    }

                    normal = vector;
                }

                // write surfel
                surf.radius() = avg_distance * 0.8;
                surf.normal() = normal;
                nodes_[i].mem_array().write_surfel(surf, k);

                // update average node radius
                if (i != old_i)
                {
                    average_radius += average_radius_for_node/nodes_[old_i].mem_array().length();
                    average_radius_for_node = 0.0;
                    old_i = i;
                }
                else
                {
                    average_radius_for_node += avg_distance * 0.8;
                }
            }
        }
    }

    average_radius /= nodes_.size();

    // filter outlier radii

    std::cout << std::endl << std::endl << "filter outlier radii" << std::endl;

    real max_radius = average_radius * 15.0;

    counter = 0;
    percentage = 0;

    for (size_t i = first_leaf_; i < nodes_.size(); ++i)
    {
        for (size_t k = 0; k < max_surfels_per_node_; ++k)
        {
            if (k < nodes_[i].mem_array().length())
            {
                ++counter;
                uint16_t new_percentage = int(float(counter)/(number_of_leaves*max_surfels_per_node_) * 100);
                if (percentage + 1 == new_percentage)
                {
                    percentage = new_percentage;
                    std::cout << "\r" << percentage << "% processed" << std::flush;
                }

                surfel surf = nodes_[i].mem_array().read_surfel(k);

                if (surf.radius() > max_radius)
                {
                    surf.radius() = max_radius;
                    nodes_[i].mem_array().write_surfel(surf, k);
                }
            }
        }
    }


    for (size_t i = first_leaf_; i < nodes_.size(); ++i)
    {
        nodes_[i].flush_to_disk(true);
    }

    std::cout << std::endl << std::endl;

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

            // compute radius
            real radius = radius_computation_strategy.compute_radius(*this, surfel_id_t(source_node->node_id(), k));

            // compute normal
            vec3f normal = normal_computation_strategy.compute_normal(*this, surfel_id_t(source_node->node_id(), k));            

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
    for (unsigned int i = 0; i < num_nearest_neighbours; ++i) {
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

    for (unsigned int i = 0; i < num_nearest_neighbours; ++i) {
        nni_sample_t sp;
        sp.xy_ = scm::math::vec2f(coords[2*i+0], coords[2*i+1]);
        Point2 p(sp.xy_.x, sp.xy_.y);
        auto vertex_handle = delaunay_triangulation.insert(p);
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

    std::vector<std::pair<unsigned int, double> > nni_weights;

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
get_natural_neighbours(const surfel_id_t& target_surfel,
                       const uint32_t num_nearest_neighbours) const {

    std::vector<std::pair<surfel_id_t, real>> nearest_neighbour_ids =
    get_nearest_neighbours(target_surfel, num_nearest_neighbours);

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
upsweep(const reduction_strategy& reduction_strtgy)
{
    LOGGER_TRACE("create level temp files");

    std::vector<shared_file> level_temp_files;
    for (uint32_t i = 0; i <= depth_; ++i) {
        level_temp_files.push_back(std::make_shared<file>());
        std::string ext = ".lv" + std::to_string(i);
        level_temp_files.back()->open(add_to_path(base_path_, ext).string(),
                                      i != depth_);
    }

    LOGGER_TRACE("Compute LOD hierarchy");

    std::atomic_uint ctr{ 0 };

    #pragma omp parallel
    {
        #pragma omp single nowait
        {
            upsweep_r(nodes_[0], reduction_strtgy, level_temp_files, ctr);
        }
    }
    std::cout << std::endl << std::endl;
    LOGGER_TRACE("Total processed nodes: " << ctr.load());
    state_ = state_type::after_upsweep;
}

void bvh::
upsweep_new(const reduction_strategy& reduction_strategy, 
            const normal_computation_strategy& normal_strategy, 
            const radius_computation_strategy& radius_strategy,
            bool recompute_leaf_level)
{
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

        // Used for progress visualization.
        size_t counter = 0;
        uint16_t percentage = 0;
    

        // Iterate over nodes of current tree level.
        // First apply reduction strategy, since calculation of attributes might depend on surfel data of nodes in same level.
        // #pragma omp parallel for
        for(uint32_t node_index = first_node_of_level; node_index < last_node_of_level; ++node_index) {
            bvh_node* current_node = &nodes_.at(node_index);
            
            if(level != depth_) {
                // If a node has no data yet, calculate it based on child nodes.
                if (!current_node->is_in_core() && !current_node->is_out_of_core()) {

                    std::vector<surfel_mem_array*> child_mem_arrays;
                    for (uint8_t child_index = 0; child_index < fan_factor_; ++child_index) {
                        size_t child_id = this->get_child_id(current_node->node_id(), child_index);
                        bvh_node* child_node = &nodes_.at(child_id);

                        child_mem_arrays.push_back(&child_node->mem_array());
                    }
                    
                    real reduction_error;

                    surfel_mem_array reduction = reduction_strategy.create_lod(reduction_error, child_mem_arrays, max_surfels_per_node_, (*this), get_child_id(current_node->node_id(), 0) );
                        
                    current_node->reset(reduction);
                    current_node->set_reduction_error(reduction_error);
                }
            }
            current_node->calculate_statistics();
        }

        


            



        {
        // skip the leaf level attribute computation if it was not requested or necessary
        if( !(level == depth_ && !recompute_leaf_level) ) {

                #pragma omp parallel for
                for(uint32_t node_index = first_node_of_level; node_index < last_node_of_level; ++node_index)
                {   
                    bvh_node* current_node = &nodes_.at(node_index);


                        // Calculate and set node properties.
                        compute_normal_and_radius(current_node, normal_strategy, radius_strategy);

                        ++counter;
                        uint16_t new_percentage = int(float(counter)/(get_length_of_depth(level)) * 100);
                        if (percentage + 1 == new_percentage)
                        {
                            percentage = new_percentage;
                            std::cout << "\r" << percentage << "% processed" << std::flush;
                        }
                }

            }

            #pragma omp parallel for
            for(uint32_t node_index = first_node_of_level; node_index < last_node_of_level; ++node_index)
            {   
                bvh_node* current_node = &nodes_.at(node_index);


                basic_algorithms::surfel_group_properties props = basic_algorithms::compute_properties(current_node->mem_array(), rep_radius_algo_);

                bounding_box node_bounding_box;
                node_bounding_box.expand(props.bbox);

                if (level < depth_) {
                    for (int child_index = 0; child_index < fan_factor_; ++child_index)
                    {
                        uint32_t child_id = this->get_child_id(current_node->node_id(), child_index);
                        bvh_node* child_node = &nodes_.at(child_id);

                        node_bounding_box.expand(child_node->get_bounding_box());
                    }
                }

                current_node->set_avg_surfel_radius(props.rep_radius);
                current_node->set_centroid(props.centroid);

                current_node->set_bounding_box(node_bounding_box);
            }
            std::cout << std::endl;
        }

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

void bvh::
upsweep_r(bvh_node& node,
          const reduction_strategy& reduction_strtgy,
          std::vector<shared_file>& level_temp_files,
          std::atomic_uint& ctr)
{
    std::vector<surfel_mem_array*> child_arrays;

    bounding_box new_bounding_box;

    if (node.depth() == depth_ - 1) {
        // leaf nodes
        for (uint32_t j = 0; j < fan_factor_; ++j) {
            const uint32_t id = get_child_id(node.node_id(), j);
            assert(!nodes_[id].is_in_core());
            nodes_[id].load_from_disk();
            child_arrays.push_back(&nodes_[id].mem_array());
            new_bounding_box.expand(nodes_[id].get_bounding_box());
        }
    }
    else {
        // inner nodes
        for (uint32_t j = 0; j < fan_factor_; ++j) {
            const uint32_t id = get_child_id(node.node_id(), j);
            #pragma omp task shared(reduction_strtgy, level_temp_files, ctr)
            upsweep_r(nodes_[id], reduction_strtgy, level_temp_files, ctr);
        }
        #pragma omp taskwait
        for (uint32_t j = 0; j < fan_factor_; ++j) {
            const uint32_t id = get_child_id(node.node_id(), j);
            child_arrays.push_back(&nodes_[id].mem_array());
            new_bounding_box.expand(nodes_[id].get_bounding_box());
        }
    }
    //LOGGER_TRACE("1. LOD " << node.node_id());
    // create LOD for current node
    real reduction_error;
    node.reset(reduction_strtgy.create_lod(reduction_error, child_arrays, max_surfels_per_node_, *this, get_child_id(node.node_id(), 0) ) );

    //LOGGER_TRACE("2. Error " << node.node_id());
    auto props = basic_algorithms::compute_properties(node.mem_array(), rep_radius_algo_);
    new_bounding_box.expand(props.bbox);

    // set reduction error, average radius, and new bounding box
    node.set_reduction_error(reduction_error);
    node.set_avg_surfel_radius(props.rep_radius);
    node.set_centroid(props.centroid);
    node.set_bounding_box(new_bounding_box);

    // recompute bounding box
    //const bounding_box bb = basic_algorithms::compute_aabb(node.mem_array(), false);
    //node.set_bounding_box(bb);

    //LOGGER_TRACE("3. Offset " << node.node_id());
    // compute node offset in file
    int32_t lid = node.node_id();
    for (uint32_t i = 0; i < node.depth(); ++i)
        lid -= uint32_t(pow(fan_factor_, i));
    lid = std::max(0, lid);

    //LOGGER_TRACE("4. Flush " << node.node_id());
    // save computed node to disk
    node.flush_to_disk(level_temp_files[node.depth()], size_t(lid) * max_surfels_per_node_, false);

    //LOGGER_TRACE("5. Switch children " << node.node_id());
    // switch children to out-of-core
    for (uint32_t j = 0; j < fan_factor_; ++j) {
        const uint32_t id = get_child_id(node.node_id(), j);
        nodes_[id].mem_array().reset();
    }

    auto count = ctr.fetch_add(1u);

    if (count % 50 == 0) {
        const int p = 100 * (count + 1) / first_leaf_;
        LOGGER_TRACE("Processed nodes so far: " << count + 1
                                << " (" << p <<"%), current node_id " << node.node_id());
        std::cout << "\r" << p << "% processed" << std::flush;
    }
}

surfel_vector bvh::
remove_outliers_statistically(uint32_t num_outliers, uint16_t num_neighbours) {

    std::vector<std::vector< std::pair<surfel_id_t, real> > > intermediate_outliers;
    std::vector<bool> already_resized;

    intermediate_outliers.resize(omp_get_max_threads());
    already_resized.resize(omp_get_max_threads());

    for( unsigned i = 0; i < already_resized.size(); ++i ) {
        already_resized[i] = false;
    }

    #pragma omp parallel for
    for(uint32_t node_idx = first_leaf_; node_idx < nodes_.size(); ++node_idx) {

        size_t thread_id = omp_get_thread_num();


        if( !already_resized[thread_id] ) {
            intermediate_outliers[thread_id].reserve(num_outliers);
            already_resized[thread_id] = true;
        }

        bvh_node* current_node = &nodes_.at(node_idx);
        
        if (current_node->is_out_of_core())
        {
            current_node->load_from_disk();
        }

        for( size_t surfel_idx = 0; surfel_idx < current_node->mem_array().length(); ++surfel_idx){

            auto const nearest_neighbour_vector = get_nearest_neighbours(surfel_id_t(node_idx, surfel_idx), num_neighbours);

            double avg_dist = 0.0;

            if( nearest_neighbour_vector.size() ) {
                for( auto const& nearest_neighbour_pair : nearest_neighbour_vector ) {
                    avg_dist += nearest_neighbour_pair.second;
                }

                avg_dist /= nearest_neighbour_vector.size();
            }

            bool insert_element = false;
            if( intermediate_outliers[thread_id].size() < num_outliers ) {
                insert_element = true;

            } else if ( avg_dist > intermediate_outliers[thread_id].back().second ) {
                intermediate_outliers[thread_id].pop_back();
                insert_element = true;
            }

            if( insert_element ) {
                intermediate_outliers[thread_id].push_back( std::make_pair(surfel_id_t(node_idx, surfel_idx), avg_dist) );

                for (uint32_t k = intermediate_outliers[thread_id].size() - 1; k > 0; --k)
                {
                    if (intermediate_outliers[thread_id][k].second > intermediate_outliers[thread_id][k - 1].second)
                    {
                        std::swap(intermediate_outliers[thread_id][k], intermediate_outliers[thread_id][k - 1]);
                    }
                    else
                        break;
                }             
            }
        }


    }


    std::vector< std::pair<surfel_id_t, real> >  final_outliers;

    for (auto const& ve : intermediate_outliers) {
        //std::cout << "Vector in Slot: " << result_counter++ << " contains " << ve.size() << " elements\n";

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
