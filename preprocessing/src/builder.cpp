// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/builder.h>

#include <lamure/utils.h>
#include <lamure/memory.h>
#include <lamure/pre/bvh.h>
#include <lamure/pre/io/format_abstract.h>
#include <lamure/pre/io/format_xyz.h>
#include <lamure/pre/io/format_xyz_all.h>
#include <lamure/pre/io/format_ply.h>
#include <lamure/pre/io/format_bin.h>
#include <lamure/pre/io/converter.h>

#include <lamure/pre/normal_computation_plane_fitting.h>
#include <lamure/pre/radius_computation_average_distance.h>
#include <lamure/pre/radius_computation_natural_neighbours.h>
#include <lamure/pre/reduction_normal_deviation_clustering.h>
#include <lamure/pre/reduction_constant.h>
#include <lamure/pre/reduction_every_second.h>
#include <lamure/pre/reduction_random.h>
#include <lamure/pre/reduction_entropy.h>
#include <lamure/pre/reduction_particle_simulation.h>
#include <lamure/pre/reduction_hierarchical_clustering.h>
#include <lamure/pre/reduction_k_clustering.h>
#include <lamure/pre/reduction_spatially_subdivided_random.h>
#include <lamure/pre/reduction_pair_contraction.h>
#include <lamure/pre/reduction_hierarchical_clustering_mk5.h>

#include <cstdio>


#define CPU_TIMER auto_timer timer("CPU time: %ws wall, usr+sys = %ts CPU (%p%)\n")

namespace fs = boost::filesystem;

namespace lamure {
namespace pre
{

builder::
builder(const descriptor& desc)
    : desc_(desc),
      memory_limit_(0)
{
    base_path_ = fs::path(desc_.working_directory) 
                 / fs::path(desc_.input_file).stem().string();
}

builder::~builder()
{
}

reduction_strategy* builder::get_reduction_strategy(reduction_algorithm algo) const {
    switch (algo) {
        case reduction_algorithm::ndc:
            return new reduction_normal_deviation_clustering();
        case reduction_algorithm::constant:
            return new reduction_constant();
        case reduction_algorithm::every_second:
            return new reduction_every_second();
        case reduction_algorithm::random:
            return new reduction_random();
        case reduction_algorithm::entropy:
            return new reduction_entropy();
        case reduction_algorithm::particle_sim:
            return new reduction_particle_simulation();
        case reduction_algorithm::hierarchical_clustering:
            return new reduction_hierarchical_clustering();
        case reduction_algorithm::k_clustering:
            return new reduction_k_clustering(desc_.number_of_neighbours);
        case reduction_algorithm::spatially_subdivided_random:
            return new reduction_spatially_subdivided_random();
        case reduction_algorithm::pair:
            return new reduction_pair_contraction(desc_.number_of_neighbours);
        case reduction_algorithm::hierarchical_clustering_extended:
            return new reduction_hierarchical_clustering_mk5();
        default:
            LOGGER_ERROR("Non-implemented reduction algorithm");
            return nullptr;
    };
}

radius_computation_strategy* builder::get_radius_strategy(radius_computation_algorithm algo) const{
    switch (algo) {
        case radius_computation_algorithm::average_distance:
            return new radius_computation_average_distance(desc_.number_of_neighbours, desc_.radius_multiplier);
        case radius_computation_algorithm::natural_neighbours:
            return new radius_computation_natural_neighbours(20, 10, 3);
        default:
            LOGGER_ERROR("Non-implemented radius computation algorithm");
            return nullptr;
    };
}

normal_computation_strategy* builder::get_normal_strategy(normal_computation_algorithm algo) const{
    switch (algo) {
        case normal_computation_algorithm::plane_fitting:
            return new normal_computation_plane_fitting(desc_.number_of_neighbours);
        default:
            LOGGER_ERROR("Non-implemented normal computation algorithm");
            return nullptr;
    };
}

boost::filesystem::path builder::convert_to_binary(std::string const& input_type) const{
    std::cout << std::endl;
    std::cout << "--------------------------------" << std::endl;
    std::cout << "convert input file" << std::endl;
    std::cout << "--------------------------------" << std::endl;

    LOGGER_TRACE("convert to a binary file");
    auto input_file = fs::canonical(fs::path(desc_.input_file));
    format_abstract* format_in;
    auto binary_file = base_path_;

    if (input_type == ".xyz") {
        binary_file += ".bin";
        format_in = new format_xyz();
    }
    else if (input_type == ".xyz_all") {
        binary_file += ".bin_all";
        format_in = new format_xyzall();
    }
    else if (input_type == ".ply") {
        binary_file += ".bin";
        format_in = new format_ply();
    }
    else {
        LOGGER_ERROR("Unable to convert input file: Unknown file format");
        return false;
    }

    format_bin format_out;

    converter conv(*format_in, format_out, desc_.buffer_size);

    conv.set_surfel_callback([](surfel &s, bool& keep) { if (s.pos() == vec3r(0.0,0.0,0.0)) keep = false; });
    //conv.set_scale_factor(1);
    //conv.set_translation(vec3r(-605535.577, -5097551.573, -1468.071));

    CPU_TIMER;
    conv.convert(input_file.string(), binary_file.string());
    delete format_in;
    // LOGGER_DEBUG("Used memory: " << GetProcessUsedMemory() / 1024 / 1024 << " MiB");
    return binary_file;
}

boost::filesystem::path builder::downsweep(boost::filesystem::path input_file, uint16_t start_stage) const{
    bool performed_outlier_removal = false;
    do {
        std::string status_suffix = "";
        if ( true == performed_outlier_removal ) {
            status_suffix = " (after outlier removal)";
        }

        std::cout << std::endl;
        std::cout << "--------------------------------" << std::endl;
        std::cout << "bvh properties" << status_suffix << std::endl;
        std::cout << "--------------------------------" << std::endl;

        lamure::pre::bvh bvh(memory_limit_, desc_.buffer_size, desc_.rep_radius_algo);

        bvh.init_tree(input_file.string(),
                          desc_.max_fan_factor,
                          desc_.surfels_per_node,
                          base_path_);

        bvh.print_tree_properties();
        std::cout << std::endl;

        std::cout << "--------------------------------" << std::endl;
        std::cout << "downsweep" << status_suffix << std::endl;
        std::cout << "--------------------------------" << std::endl;
        LOGGER_TRACE("downsweep stage");

        CPU_TIMER;
        bvh.downsweep(desc_.translate_to_origin, input_file.string());

        auto bvhd_file = add_to_path(base_path_, ".bvhd");

        bvh.serialize_tree_to_file(bvhd_file.string(), true);

        if ((!desc_.keep_intermediate_files) && (start_stage < 1))
        {
            // do not remove input file
            std::remove(input_file.string().c_str());
        }

        input_file = bvhd_file;

        // LOGGER_DEBUG("Used memory: " << GetProcessUsedMemory() / 1024 / 1024 << " MiB");

        if ( 3 <= start_stage ) {
            break;
        }

        if ( performed_outlier_removal ) {
            break;
        }

        if(start_stage <= 2 ) {

            if(desc_.outlier_ratio != 0.0) {

                size_t num_outliers = desc_.outlier_ratio * (bvh.nodes().size() - bvh.first_leaf()) * bvh.max_surfels_per_node();
                size_t ten_percent_of_surfels = std::max( size_t(0.1 * (bvh.nodes().size() - bvh.first_leaf()) * bvh.max_surfels_per_node()), size_t(1) );
                num_outliers = std::min(std::max(num_outliers, size_t(1) ), ten_percent_of_surfels); // remove at least 1 surfel, for any given ratio != 0.0

                std::cout << std::endl;
                std::cout << "--------------------------------" << std::endl;
                std::cout << "outlier removal ( " << int(desc_.outlier_ratio * 100) << " percent = " << num_outliers << " surfels)" << std::endl;
                std::cout << "--------------------------------" << std::endl;
                LOGGER_TRACE("outlier removal stage");

                surfel_vector kept_surfels = bvh.remove_outliers_statistically(num_outliers, desc_.number_of_outlier_neighbours);

                format_bin format_out;

                format_abstract* dummy_format_in;

                dummy_format_in = new format_xyz();

                converter conv(*dummy_format_in, format_out, desc_.buffer_size);

                conv.set_surfel_callback([](surfel &s, bool& keep) { if (s.pos() == vec3r(0.0,0.0,0.0)) keep = false; });

                auto binary_outlier_removed_file = add_to_path(base_path_, ".bin_wo_outlier");

                conv.write_in_core_surfels_out(kept_surfels, binary_outlier_removed_file.string());

                delete dummy_format_in;

                bvh.reset_nodes();

                input_file = fs::canonical(binary_outlier_removed_file);

                performed_outlier_removal = true;
            } else {
                break;
            }

        }

    } while( true );

    return input_file;
}

boost::filesystem::path builder::upsweep(boost::filesystem::path input_file,
                     uint16_t start_stage,
                     reduction_strategy const* reduction_strategy,
                     normal_computation_strategy const* normal_comp_strategy,
                     radius_computation_strategy const* radius_comp_strategy) const {
    std::cout << std::endl;
    std::cout << "--------------------------------" << std::endl;
    std::cout << "upsweep" << std::endl;
    std::cout << "--------------------------------" << std::endl;
    LOGGER_TRACE("upsweep stage");

    lamure::pre::bvh bvh(memory_limit_, desc_.buffer_size, desc_.rep_radius_algo);

    if (!bvh.load_tree(input_file.string())) {
        return boost::filesystem::path{};
    }

    if (bvh.state() != bvh::state_type::after_downsweep) {
        LOGGER_ERROR("Wrong processing state!");
        return boost::filesystem::path{};
    }

    CPU_TIMER;

    // perform upsweep
    bvh.upsweep(*reduction_strategy, 
                *normal_comp_strategy, 
                *radius_comp_strategy,
                desc_.compute_normals_and_radii,
                desc_.resample);


    delete reduction_strategy;
    delete normal_comp_strategy;
    delete radius_comp_strategy;

    //write resampled leaf level out
    format_xyz format_out;
    format_abstract* dummy_format_in;
    dummy_format_in = new format_xyz();
    auto xyz_res_file = add_to_path(base_path_, ".xyz_res");
    converter conv(*dummy_format_in, format_out, desc_.buffer_size);
    surfel_vector resampled_ll = bvh.get_resampled_leaf_lv_surfels();
    conv.write_in_core_surfels_out(resampled_ll, xyz_res_file.string());
    delete dummy_format_in;


    auto bvhu_file = add_to_path(base_path_, ".bvhu");
    bvh.serialize_tree_to_file(bvhu_file.string(), true);

    if ((!desc_.keep_intermediate_files) && (start_stage < 2)) {
        std::remove(input_file.string().c_str());
    }

    input_file = bvhu_file;
// LOGGER_DEBUG("Used memory: " << GetProcessUsedMemory() / 1024 / 1024 << " MiB");
    return input_file;
}

bool builder::reserialize(boost::filesystem::path const& input_file, uint16_t start_stage) const {
    std::cout << std::endl;
    std::cout << "--------------------------------" << std::endl;
    std::cout << "serialize to file" << std::endl;
    std::cout << "--------------------------------" << std::endl;

    lamure::pre::bvh bvh(memory_limit_, desc_.buffer_size, desc_.rep_radius_algo);
    if (!bvh.load_tree(input_file.string())) {
        return false;
    }
    if (bvh.state() != bvh::state_type::after_upsweep) {
        LOGGER_ERROR("Wrong processing state!");
        return false;
    }

    CPU_TIMER;
    auto lod_file = add_to_path(base_path_, ".lod");
    auto kdn_file = add_to_path(base_path_, ".bvh");

    std::cout << "serialize surfels to file" << std::endl;
    bvh.serialize_surfels_to_file(lod_file.string(), desc_.buffer_size);

    std::cout << "serialize bvh to file" << std::endl << std::endl;
    bvh.serialize_tree_to_file(kdn_file.string(), false);

    if ((!desc_.keep_intermediate_files) && (start_stage < 3)) {
        std::remove(input_file.string().c_str());
        bvh.reset_nodes();
    }
    // LOGGER_DEBUG("Used memory: " << GetProcessUsedMemory() / 1024 / 1024 << " MiB");
    return true;
}

bool builder::
construct()
{
    // compute memory parameters
    const size_t memory_budget = get_total_memory() * desc_.memory_ratio;
    const size_t occupied = get_total_memory() - get_available_memory();

    if (occupied >= memory_budget) {
        LOGGER_ERROR("Memory ratio is too small");
        return false;
    }

    memory_limit_ = memory_budget - occupied;

    LOGGER_INFO("Total physical memory: " << get_total_memory() / 1024 / 1024 << " MiB");
    LOGGER_INFO("Memory limit: " << memory_limit_ / 1024 / 1024 << " MiB");
    LOGGER_INFO("Precision for storing coordinates and radii: " << std::string((sizeof(real) == 8) ? "double" : "single"));

    uint16_t start_stage = 0;
    uint16_t final_stage = desc_.final_stage;

    auto input_file = fs::canonical(fs::path(desc_.input_file));
    const std::string input_file_type = input_file.extension().string();

    if (input_file_type == ".xyz" || 
        input_file_type == ".ply" || 
        input_file_type == ".bin")
        desc_.compute_normals_and_radii = true;

    if (input_file_type == ".xyz" || 
        input_file_type == ".xyz_all" || 
        input_file_type == ".ply")
        start_stage = 0;
    else if (input_file_type == ".bin" || input_file_type == ".bin_all")
        start_stage = 1;
    else if (input_file_type == ".bin_wo_outlier")
        start_stage = 3;
    else if (input_file_type == ".bvhd")
        start_stage = 4;
    else if (input_file_type == ".bvhu")
        start_stage = 5;
    else {
        LOGGER_ERROR("Unknown input file format");
        return false;
    }

    // init algorithms
    reduction_strategy *reduction_strategy = get_reduction_strategy(desc_.reduction_algo);
    normal_computation_strategy *normal_comp_strategy = get_normal_strategy(desc_.normal_computation_algo);
    radius_computation_strategy *radius_comp_strategy = get_radius_strategy(desc_.radius_computation_algo);

    // convert to binary file
    if ((0 >= start_stage) && (0 <= final_stage)) {
        input_file = convert_to_binary(input_file_type);
        if(input_file.empty()) return false;
    }

    // downsweep (create bvh)
    if ((3 >= start_stage) && (3 <= final_stage)) {
        input_file = downsweep(input_file, start_stage);
        if(input_file.empty()) return false;
    }

    // upsweep (create LOD)
    if ((4 >= start_stage) && (4 <= final_stage)) {
       input_file = upsweep(input_file, start_stage, reduction_strategy, normal_comp_strategy, radius_comp_strategy);
       if(input_file.empty()) return false;
    }

    // serialize to file
    if ((5 >= start_stage) && (5 <= final_stage)) {
        bool reserialize_success = reserialize(input_file, start_stage);
        if(!reserialize_success) return false;
    }
    return true;
}

} // namespace pre
} // namespace lamure
