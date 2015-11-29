// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/bvh.h>
#include <lamure/pre/bvh_stream.h>
#include <lamure/pre/basic_algorithms.h>
#include <lamure/pre/reduction_strategy.h>
#include <lamure/pre/serialized_surfel.h>
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

void  Bvh::
InitTree(const std::string& surfels_input_file,
         const uint32_t max_fan_factor,
         const size_t desired_surfels_per_node,
         const boost::filesystem::path& base_path)
{
    assert(state_ == State::Null);
    assert(max_fan_factor >= 2);
    assert(desired_surfels_per_node >= 5);

    base_path_ = base_path;

    // get number of surfels
    File input;
    input.Open(surfels_input_file);
    size_t num_surfels = input.GetSize();
    input.Close();

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

    nodes_ = std::vector<BvhNode>(num_nodes);
    first_leaf_ = nodes_.size() - std::pow(fan_factor_, depth_);
    state_ = State::Empty;
}

bool Bvh::
LoadTree(const std::string& kdn_input_file)
{
    assert(state_ == State::Null);

    BvhStream bvh_stream;
    bvh_stream.ReadBvh(kdn_input_file, *this);

    LOGGER_INFO("Load bvh: \"" << kdn_input_file << "\". State: " << StateToString(state_));
    return true;
}

uint32_t Bvh::
GetChildId(const uint32_t node_id, const uint32_t child_index) const
{
    return node_id*fan_factor_ + 1 + child_index;
}

uint32_t Bvh::
GetParentId(const uint32_t node_id) const
{
    //TODO: might be better to assert on root node instead
    if (node_id == 0) return 0;

    if (node_id % fan_factor_ == 0)
        return node_id/fan_factor_ - 1;
    else
        return (node_id + fan_factor_ - (node_id % fan_factor_))
               / fan_factor_ - 1;
}

std::pair<NodeIdType, NodeIdType> Bvh::
GetNodeRanges(const uint32_t depth) const
{
    assert(depth >= 0 && depth <= depth_);

    NodeIdType first = 0, count = 1;
    for (NodeIdType i = 1; i <= depth; ++i) {
        first += count;
        count *= fan_factor_;
    }
    return std::make_pair(first, count);
}

void Bvh::
PrintTreeProperties() const
{
    LOGGER_INFO("Fan-out factor: " << int(fan_factor_));
    LOGGER_INFO("Depth: " << depth_);
    LOGGER_INFO("Number of nodes: " << nodes_.size());
    LOGGER_INFO("Max surfels per node: " << max_surfels_per_node_);
    LOGGER_INFO("First leaf node id: " << first_leaf_);
}

void Bvh::
Downsweep(bool adjust_translation,
          const std::string& surfels_input_file,
          bool bin_all_file_extension)
{
    assert(state_ == State::Empty);

    size_t in_core_surfel_capacity = memory_limit_ / sizeof(Surfel);

    size_t disk_leaf_destination = 0,
           slice_left = 0,
           slice_right = 0;

    LOGGER_INFO("Build bvh for \"" << surfels_input_file << "\"");

    // open input file and leaf level file
    SharedFile input_file_disk_access = std::make_shared<File>();
    input_file_disk_access->Open(surfels_input_file);

    SharedFile leaf_level_access = std::make_shared<File>();
    std::string file_extension = ".lv" + std::to_string(depth_);
    if (bin_all_file_extension)
        file_extension = ".bin_all";
    leaf_level_access->Open(AddToPath(base_path_, file_extension).string(), true);

    // instantiate root surfel array
    SurfelDiskArray input(input_file_disk_access, 0, input_file_disk_access->GetSize());
    LOGGER_INFO("Total number of surfels: " << input.length());

    // compute depth at which we can switch to in-core
    uint32_t final_depth = std::max(0.0,
            std::ceil(std::log(input.length() / double(in_core_surfel_capacity)) /
                 std::log(double(fan_factor_))));

    assert(final_depth <= depth_);

    LOGGER_INFO("Tree depth to switch in-core: " << final_depth);

    // construct root node
    nodes_[0] = BvhNode(0, 0, BoundingBox(), input);
    BoundingBox input_bb;

    // check if the root can be switched to in-core
    if (final_depth == 0) {
        LOGGER_TRACE("Compute root bounding box in-core");
        nodes_[0].LoadFromDisk();
        input_bb = BasicAlgorithms::ComputeAABB(nodes_[0].mem_array());

    }
    else {
        LOGGER_TRACE("Compute root bounding box out-of-core");
        input_bb = BasicAlgorithms::ComputeAABB(nodes_[0].disk_array(),
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
            BasicAlgorithms::TranslateSurfels(nodes_[0].mem_array(), -translation);
        }
        else {
            BasicAlgorithms::TranslateSurfels(nodes_[0].disk_array(), -translation, buffer_size_);
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
            BvhNode& current_node = nodes_[nid];
            // make sure that current node is out-of-core
            assert(current_node.IsOOC());

            // split and compute child bounding boxes
            BasicAlgorithms::SplittedArray<SurfelDiskArray> surfel_arrays;

            BasicAlgorithms::SortAndSplit(current_node.disk_array(),
                                          surfel_arrays,
                                          current_node.bounding_box(),
                                          current_node.bounding_box().GetLongestAxis(),
                                          fan_factor_,
                                          memory_limit_);

            // iterate through children
            for (size_t i = 0; i < surfel_arrays.size(); ++i) {
                uint32_t child_id = GetChildId(nid, i);
                nodes_[child_id] = BvhNode(child_id, level + 1,
                                           surfel_arrays[i].second,
                                           surfel_arrays[i].first);
                if (nid == slice_left && i == 0)
                    new_slice_left = child_id;
                if (nid == slice_right && i == surfel_arrays.size() - 1)
                    new_slice_right = child_id;
            }

            current_node.Reset();

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
        BvhNode& current_node = nodes_[nid];

        // make sure that current node is out-of-core and switch to in-core (unless root node)
        if (nid > 0) {
            assert(current_node.IsOOC());
            current_node.LoadFromDisk();
        }
        LOGGER_TRACE("Process subbvh in-core at node " << nid);
        // process subbvh and save leafs
        DownsweepSubtreeInCore(current_node, disk_leaf_destination, processed_nodes,
                               percent_processed,
                               leaf_level_access);
    }
    //std::cout << std::endl << std::endl;

    input_file_disk_access->Close();
    state_ = State::AfterDownsweep;
}

void Bvh::
DownsweepSubtreeInCore( const BvhNode& node,
                        size_t& disk_leaf_destination,
                        uint32_t& processed_nodes,
                        uint8_t& percent_processed,
                        SharedFile leaf_level_access)
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
            BvhNode& current_node = nodes_[nid];
            // make sure that current node is in-core
            assert(current_node.IsIC());

            // split and compute child bounding boxes
            BasicAlgorithms::SplittedArray<SurfelMemArray> surfel_arrays;
            BasicAlgorithms::SortAndSplit(current_node.mem_array(),
                                          surfel_arrays,
                                          current_node.bounding_box(),
                                          current_node.bounding_box().GetLongestAxis(),
                                          fan_factor_,
                                          (slice_right - slice_left) < sort_parallelizm_thres);

            // iterate through children
            for (size_t i = 0; i < surfel_arrays.size(); ++i) {
                uint32_t child_id = GetChildId(nid, i);
                nodes_[child_id] = BvhNode(child_id, level + 1,
                                           surfel_arrays[i].second,
                                           surfel_arrays[i].first);
                if (nid == slice_left && i == 0)
                    new_slice_left = child_id;
                if (nid == slice_right && i == surfel_arrays.size() - 1)
                    new_slice_right = child_id;
            }

            current_node.Reset();

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
        BvhNode& current_node = nodes_[nid];
        auto props = BasicAlgorithms::ComputeProperties(current_node.mem_array(), rep_radius_algo_);
        current_node.set_avg_surfel_radius(props.rep_radius);
        current_node.set_centroid(props.centroid);
        current_node.set_bounding_box(props.bounding_box);
    }

    LOGGER_TRACE("Save leaves to disk");
    // save leaves to disk
    for (size_t nid = slice_left; nid <= slice_right; ++nid) {
        BvhNode& current_node = nodes_[nid];
        current_node.FlushToDisk(leaf_level_access,
                                 disk_leaf_destination, true);
        disk_leaf_destination += current_node.disk_array().length();
    }
}

void Bvh::ComputeNormalsAndRadii(const uint16_t number_of_neighbours)
{
    size_t number_of_leaves = std::pow(fan_factor_, depth_);

    // load from disk
    for (size_t i = first_leaf_; i < nodes_.size(); ++i)
    {
        nodes_[i].LoadFromDisk();
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
                std::vector<std::pair<Surfel, real>> neighbours = GetNearestNeighbours(i, k, number_of_neighbours);
                Surfel surfel = nodes_[i].mem_array().ReadSurfel(k);

                // compute radius
                real avg_distance = 0.0;

                for (auto neighbour : neighbours)
                {
                    avg_distance += sqrt(neighbour.second);
                }

                avg_distance /= neighbours.size();

                // compute normal
                // see: http://missingbytes.blogspot.com/2012/06/fitting-plane-to-point-cloud.html

                vec3r center = surfel.pos();
                vec3f normal(1.0, 0.0 , 0.0);

                real sum_x_x = 0.0, sum_x_y = 0.0, sum_x_z = 0.0;
                real sum_y_y = 0.0, sum_y_z = 0.0;
                real sum_z_z = 0.0;

                for (auto neighbour : neighbours)
                {
                    vec3r neighbour_pos = neighbour.first.pos();

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
                surfel.radius() = avg_distance * 0.8;
                surfel.normal() = normal;
                nodes_[i].mem_array().WriteSurfel(surfel, k);

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

                Surfel surfel = nodes_[i].mem_array().ReadSurfel(k);

                if (surfel.radius() > max_radius)
                {
                    surfel.radius() = max_radius;
                    nodes_[i].mem_array().WriteSurfel(surfel, k);
                }
            }
        }
    }


    for (size_t i = first_leaf_; i < nodes_.size(); ++i)
    {
        nodes_[i].FlushToDisk(true);
    }

    std::cout << std::endl << std::endl;

}

void Bvh::compute_normal_and_radius(const size_t node, 
                                    const size_t surfel,
                                    const NormalRadiiStrategy& normal_radii_strategy){


    //set sonstant number of neighbours for test
    //use desc. !? 
    const uint32_t number_of_neighbours = 40;
    size_t node_id = node;
    size_t surfel_id = surfel;
    std::vector<std::pair<Surfel, real>>  neighbours;


   /*
    neighbours = GetNearestNeighbours(node_id, surfel_id, number_of_neighbours)

    if (current_surfel < nodes_[node_id].mem_array().length()){
        Surfel surfel = nodes_[node_id].mem_array().ReadSurfel(surfel_id);

        surfel.normal() = normal_radii_strategy.compute_normal(node_id, surfel_id, neighbours);
        surfel.radius() = normal_radii_strategy.compute_radius(node_id, surfel_id, neighbours);

        nodes_[node_id].mem_array().WriteSurfel(surfel, surfel_id);
    };

    */
    

    
}

void Bvh::GetDescendantLeaves(
     const size_t node,
     std::vector<size_t>& result,
     const size_t first_leaf,
     const std::unordered_set<size_t>& excluded_leaves) const
{
    if (node < first_leaf) // inner node
    {
        for (uint16_t i = 0; i < fan_factor_; ++i)
        {
            GetDescendantLeaves(GetChildId(node, i), result, first_leaf, excluded_leaves);
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

void Bvh::GetDescendantNodes(
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
            GetDescendantNodes(GetChildId(node, i), result, desired_depth, excluded_nodes);
        }
    }
}

std::vector<std::pair<Surfel, real>> Bvh::
GetNearestNeighbours(
    const size_t node,
    const size_t surfel,
    const uint32_t number_of_neighbours) const
{
    size_t current_node = node;
    std::unordered_set<size_t> processed_leaves;
    vec3r center = nodes_[node].mem_array().ReadSurfel(surfel).pos();

    std::vector<std::pair<Surfel, real>> candidates;
    real max_candidate_distance = std::numeric_limits<real>::infinity();
    Sphere candidates_sphere;

    size_t number_of_leaves = pow(fan_factor_, depth_);
    size_t first_leaf = nodes_.size() - number_of_leaves;

    // check own node

    for (size_t i = 0; i < nodes_[current_node].mem_array().length(); ++i)
    {
        if (i != surfel)
        {
            Surfel const& current_surfel = nodes_[current_node].mem_array().ReadSurfel(i);
            real distance_to_center = scm::math::length_sqr(center - current_surfel.pos());

            if (candidates.size() < number_of_neighbours || (distance_to_center < max_candidate_distance))
            {
                if (candidates.size() == number_of_neighbours)
                    candidates.pop_back();

                candidates.push_back(std::make_pair(current_surfel, distance_to_center));

                for (uint16_t k = candidates.size() - 1; k > 0; --k)
                {
                    if (candidates[k].second < candidates[k - 1].second)
                    {
                        std::pair<Surfel, real> temp = candidates [k - 1];
                        candidates[k - 1] = candidates[k];
                        candidates[k] = temp;
                    }
                    else
                        break;
                }

                max_candidate_distance = candidates.back().second;

            }

        }

    }

    processed_leaves.insert(current_node);
    candidates_sphere = Sphere(center, sqrt(max_candidate_distance));

    // check rest of kd-bvh
    while ( (!nodes_[current_node].bounding_box().Contains(candidates_sphere)) &&
            (current_node != 0) )
    {
        current_node = GetParentId(current_node);

        std::vector<size_t> unvisited_descendant_leaves;

        GetDescendantNodes(current_node, unvisited_descendant_leaves, depth_, processed_leaves);

        for (auto leaf : unvisited_descendant_leaves)
        {

            if (candidates_sphere.IsInside(nodes_[leaf].bounding_box()))
            {

                assert(nodes_[leaf].IsOOC());

                for (size_t i = 0; i < nodes_[leaf].mem_array().length(); ++i)
                {
                    if (!(leaf == node && i == surfel))
                    {
                        Surfel current_surfel = nodes_[leaf].mem_array().ReadSurfel(i);
                        real distance_to_center = scm::math::length_sqr(center - current_surfel.pos());

                        if (candidates.size() < number_of_neighbours || (distance_to_center < max_candidate_distance))
                        {
                            if (candidates.size() == number_of_neighbours)
                                candidates.pop_back();

                            candidates.push_back(std::make_pair(current_surfel, distance_to_center));

                            for (uint16_t k = candidates.size() - 1; k > 0; --k)
                            {
                                if (candidates[k].second < candidates[k - 1].second)
                                {
                                    std::pair<Surfel, real> temp = candidates [k - 1];
                                    candidates[k - 1] = candidates[k];
                                    candidates[k] = temp;
                                }
                                else
                                    break;
                            }

                            max_candidate_distance = candidates.back().second;

                        }
                    }

                }

                processed_leaves.insert(leaf);
                candidates_sphere = Sphere(center, sqrt(max_candidate_distance));

            }

        }

    }

    return candidates;
}



void Bvh::
Upsweep(const ReductionStrategy& reduction_strategy)
{
    LOGGER_TRACE("Create level temp files");

    std::vector<SharedFile> level_temp_files;
    for (uint32_t i = 0; i <= depth_; ++i) {
        level_temp_files.push_back(std::make_shared<File>());
        std::string ext = ".lv" + std::to_string(i);
        level_temp_files.back()->Open(AddToPath(base_path_, ext).string(),
                                      i != depth_);
    }

    LOGGER_TRACE("Compute LOD hierarchy");

    std::atomic_uint ctr{ 0 };

    #pragma omp parallel
    {
        #pragma omp single nowait
        {
            UpsweepR(nodes_[0], reduction_strategy, level_temp_files, ctr);
        }
    }
    std::cout << std::endl << std::endl;
    LOGGER_TRACE("Total processed nodes: " << ctr.load());
    state_ = State::AfterUpsweep;
}



void Bvh::
UpsweepR(BvhNode& node,
         const ReductionStrategy& reduction_strategy,
         std::vector<SharedFile>& level_temp_files,
         std::atomic_uint& ctr)
{
    std::vector<SurfelMemArray*> child_arrays;

    BoundingBox new_bounding_box;

    if (node.depth() == depth_ - 1) {
        // leaf nodes
        for (uint32_t j = 0; j < fan_factor_; ++j) {
            const uint32_t id = GetChildId(node.node_id(), j);
            assert(!nodes_[id].IsIC());
            nodes_[id].LoadFromDisk();
            child_arrays.push_back(&nodes_[id].mem_array());
            new_bounding_box.Expand(nodes_[id].bounding_box());
        }
    }
    else {
        // inner nodes
        for (uint32_t j = 0; j < fan_factor_; ++j) {
            const uint32_t id = GetChildId(node.node_id(), j);
            #pragma omp task shared(reduction_strategy, level_temp_files, ctr)
            UpsweepR(nodes_[id], reduction_strategy, level_temp_files, ctr);
        }
        #pragma omp taskwait
        for (uint32_t j = 0; j < fan_factor_; ++j) {
            const uint32_t id = GetChildId(node.node_id(), j);
            child_arrays.push_back(&nodes_[id].mem_array());
            new_bounding_box.Expand(nodes_[id].bounding_box());
        }
    }
    //LOGGER_TRACE("1. LOD " << node.node_id());
    // create LOD for current node
    real reduction_error;
    node.Reset(reduction_strategy.CreateLod(reduction_error, child_arrays, max_surfels_per_node_));

    //LOGGER_TRACE("2. Error " << node.node_id());
    auto props = BasicAlgorithms::ComputeProperties(node.mem_array(), rep_radius_algo_);
    new_bounding_box.Expand(props.bounding_box);

    // set reduction error, average radius, and new bounding box
    node.set_reduction_error(reduction_error);
    node.set_avg_surfel_radius(props.rep_radius);
    node.set_centroid(props.centroid);
    node.set_bounding_box(new_bounding_box);

    // recompute bounding box
    //const BoundingBox bb = BasicAlgorithms::ComputeAABB(node.mem_array(), false);
    //node.set_bounding_box(bb);

    //LOGGER_TRACE("3. Offset " << node.node_id());
    // compute node offset in file
    int32_t lid = node.node_id();
    for (uint32_t i = 0; i < node.depth(); ++i)
        lid -= uint32_t(pow(fan_factor_, i));
    lid = std::max(0, lid);

    //LOGGER_TRACE("4. Flush " << node.node_id());
    // save computed node to disk
    node.FlushToDisk(level_temp_files[node.depth()], size_t(lid) * max_surfels_per_node_, false);

    //LOGGER_TRACE("5. Switch children " << node.node_id());
    // switch children to out-of-core
    for (uint32_t j = 0; j < fan_factor_; ++j) {
        const uint32_t id = GetChildId(node.node_id(), j);
        nodes_[id].mem_array().Reset();
    }

    auto count = ctr.fetch_add(1u);

    if (count % 50 == 0) {
        const int p = 100 * (count + 1) / first_leaf_;
        LOGGER_TRACE("Processed nodes so far: " << count + 1
                                << " (" << p <<"%), current node_id " << node.node_id());
        std::cout << "\r" << p << "% processed" << std::flush;
    }
}

void Bvh::
SerializeTreeToFile(const std::string& output_file,
                    bool write_intermediate_data)
{
    LOGGER_TRACE("Serialize bvh to file: \"" << output_file << "\"");

    BvhStream bvh_stream;
    bvh_stream.WriteBvh(output_file, *this, write_intermediate_data);

}


void Bvh::
SerializeSurfelsToFile(const std::string& output_file, const size_t buffer_size) const
{
    LOGGER_TRACE("Serialize surfels to file: \"" << output_file << "\"");
    NodeSerializer serializer(max_surfels_per_node_, buffer_size);
    serializer.Open(output_file);
    serializer.SerializeNodes(nodes_);
}

void Bvh::
ResetNodes()
{
    for (auto& n: nodes_) {
        if (n.IsOOC() && n.disk_array().file().use_count() == 1) {
            n.disk_array().file()->Close(true);
        }
        n.Reset();
    }
}

std::string Bvh::
StateToString(State state)
{
    std::map<State, std::string> StateMap = {
        {State::Null,            "Null"},
        {State::Empty,           "Empty"},
        {State::AfterDownsweep,  "AfterDownsweep"},
        {State::AfterUpsweep,    "AfterUpsweep"},
        {State::Serialized,      "Serialized"}
    };
    return StateMap[state];
}

} // namespace pre
} // namespace lamure
