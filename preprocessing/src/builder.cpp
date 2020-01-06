// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
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
#include <lamure/pre/io/format_xyz_bin.h>
#include <lamure/pre/io/format_xyz_cpn.h>
#include <lamure/pre/io/format_xyz_grey.h>
#include <lamure/pre/io/format_ply.h>
#include <lamure/pre/io/format_bin.h>
#include <lamure/pre/io/converter.h>
#include <lamure/pre/io/format_xyz_prov.h>

#include <lamure/pre/normal_computation_plane_fitting.h>
#include <lamure/pre/radius_computation_average_distance.h>
#include <lamure/pre/radius_computation_natural_neighbours.h>
#include <lamure/pre/reduction_normal_deviation_clustering.h>
#include <lamure/pre/reduction_normal_deviation_clustering_provenance.h>
#include <lamure/pre/reduction_constant.h>
#include <lamure/pre/reduction_every_second.h>
#ifdef CMAKE_OPTION_ENABLE_ALTERNATIVE_STRATEGIES
#include <lamure/pre/reduction_random.h>
#include <lamure/pre/reduction_entropy.h>
#include <lamure/pre/reduction_particle_simulation.h>
#include <lamure/pre/reduction_hierarchical_clustering.h>
#include <lamure/pre/reduction_k_clustering.h>
#include <lamure/pre/reduction_spatially_subdivided_random.h>
#include <lamure/pre/reduction_pair_contraction.h>
#include <lamure/pre/reduction_hierarchical_clustering_mk5.h>
#endif
#include <cstdio>
#include <fstream>


#define CPU_TIMER auto_timer timer("CPU time: %ws wall, usr+sys = %ts CPU (%p%)\n")

namespace fs = boost::filesystem;

namespace lamure
{
namespace pre
{

builder::
builder(const descriptor &desc)
    : desc_(desc),
      memory_limit_(0)
{
    memory_limit_ = calculate_memory_limit();

    base_path_ = fs::path(desc_.working_directory)
        / fs::path(desc_.input_file).stem().string();
}

builder::~builder()
{
}

reduction_strategy *builder::get_reduction_strategy(reduction_algorithm algo) const
{
    switch (algo) {
        case reduction_algorithm::ndc:
            return new reduction_normal_deviation_clustering();
        case reduction_algorithm::ndc_prov:
            return new reduction_normal_deviation_clustering_provenance();
        case reduction_algorithm::constant:
            return new reduction_constant();
        case reduction_algorithm::every_second:
            return new reduction_every_second();
#ifdef CMAKE_OPTION_ENABLE_ALTERNATIVE_STRATEGIES
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
#endif
        default:
            LOGGER_ERROR("Non-implemented reduction algorithm");
            return nullptr;
    };
}

radius_computation_strategy *builder::get_radius_strategy(radius_computation_algorithm algo) const
{
    switch (algo) {
        case radius_computation_algorithm::average_distance:return new radius_computation_average_distance(desc_.number_of_neighbours, desc_.radius_multiplier);
        case radius_computation_algorithm::natural_neighbours:return new radius_computation_natural_neighbours(20, 10, 3);
        default:LOGGER_ERROR("Non-implemented radius computation algorithm");
            return nullptr;
    };
}

normal_computation_strategy *builder::get_normal_strategy(normal_computation_algorithm algo) const
{
    switch (algo) {
        case normal_computation_algorithm::plane_fitting:return new normal_computation_plane_fitting(desc_.number_of_neighbours);
        default:LOGGER_ERROR("Non-implemented normal computation algorithm");
            return nullptr;
    };
}

boost::filesystem::path builder::convert_to_binary(std::string const& input_filename, std::string const &input_type) const
{
    std::cout << std::endl;
    std::cout << "--------------------------------" << std::endl;
    std::cout << "convert input file" << std::endl;
    std::cout << "--------------------------------" << std::endl;

    LOGGER_TRACE("convert " << input_type << " file to a binary file");
    auto input_file = fs::canonical(fs::path(input_filename));
    std::shared_ptr<format_abstract> format_in{};
    auto binary_file = base_path_;

    if (input_type == ".xyz_prov") {
        binary_file += ".bin_prov";
        format_xyz_prov::convert(input_filename, binary_file.string(), true);
        return binary_file;
    }
    else if (input_type == ".prov") {
        binary_file += ".bin_prov";
        format_xyz_prov::convert(input_filename, binary_file.string(), false);
        return binary_file;
    }
    else if (input_type == ".xyz") {
        binary_file += ".bin";
        format_in = std::unique_ptr<format_xyz>{new format_xyz()};
    }
    else if (input_type == ".xyz_all") {
        binary_file += ".bin_all";
        format_in = std::unique_ptr<format_xyzall>{new format_xyzall()};
    }
    else if (input_type == ".xyz_bin") {
        binary_file += ".bin";
        format_in = std::unique_ptr<format_xyz_bin>{new format_xyz_bin()};
    }
    else if (input_type == ".xyz_cpn") {
        binary_file += ".bin";
        format_in = std::unique_ptr<format_xyz_cpn>{new format_xyz_cpn()};
    }
    else if (input_type == ".ply") {
        binary_file += ".bin";
        format_in = std::unique_ptr<format_ply>{new format_ply()};
    }
    else if (input_type == ".xyz_grey") {
       binary_file += ".bin";
       format_in = std::unique_ptr<format_xyz_grey>{new format_xyz_grey()};   
    }
    else {
        LOGGER_ERROR("Unable to convert input file: Unknown file format");
        return boost::filesystem::path{};
    }

    format_bin format_out;

    converter conv(*format_in, format_out, desc_.buffer_size);

    conv.set_surfel_callback([](surfel &s, bool &keep)
                             { if (s.pos() == vec3r(0.0, 0.0, 0.0)) keep = false; });

    CPU_TIMER;
    conv.convert(input_file.string(), binary_file.string());
    return binary_file;
}

boost::filesystem::path builder::downsweep(boost::filesystem::path input_file, uint16_t start_stage) const
{
    bool performed_outlier_removal = false;
    do {
        std::string status_suffix = "";
        if (true == performed_outlier_removal) {
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
        bvh.downsweep(desc_.translate_to_origin, input_file.string(), desc_.prov_file);

        auto bvhd_file = add_to_path(base_path_, ".bvhd");

        bvh.serialize_tree_to_file(bvhd_file.string(), true);

        if ((!desc_.keep_intermediate_files) && (start_stage < 1)) {
            // do not remove input file
            std::remove(input_file.string().c_str());
        }

        input_file = bvhd_file;

        // LOGGER_DEBUG("Used memory: " << GetProcessUsedMemory() / 1024 / 1024 << " MiB");

        if (3 <= start_stage) {
            break;
        }

        if (performed_outlier_removal) {
            break;
        }

        if (start_stage <= 2) {

            if (desc_.outlier_ratio != 0.0) {

                size_t num_outliers = desc_.outlier_ratio * (bvh.nodes().size() - bvh.first_leaf()) * bvh.max_surfels_per_node();

                size_t num_all_surfels = std::max(size_t((bvh.nodes().size() - bvh.first_leaf()) * bvh.max_surfels_per_node()), size_t(1));
                num_outliers = std::min(std::max(num_outliers, size_t(1)), num_all_surfels); // remove at least 1 surfel, for any given ratio != 0.0


                std::cout << std::endl;
                std::cout << "--------------------------------" << std::endl;
                std::cout << "outlier removal ( " << int(desc_.outlier_ratio * 100) << " percent = " << num_outliers << " surfels)" << std::endl;
                std::cout << "--------------------------------" << std::endl;
                LOGGER_TRACE("outlier removal stage");

                surfel_vector kept_surfels = bvh.remove_outliers_statistically(num_outliers, desc_.number_of_outlier_neighbours);

                format_bin format_out;

                std::unique_ptr<format_abstract> dummy_format_in{new format_xyz()};

                converter conv(*dummy_format_in, format_out, desc_.buffer_size);

                conv.set_surfel_callback([](surfel &s, bool &keep)
                                         { if (s.pos() == vec3r(0.0, 0.0, 0.0)) keep = false; });

                auto binary_outlier_removed_file = add_to_path(base_path_, ".bin_wo_outlier");

                conv.write_in_core_surfels_out(kept_surfels, binary_outlier_removed_file.string());

                bvh.reset_nodes();

                input_file = fs::canonical(binary_outlier_removed_file);

                performed_outlier_removal = true;
            }
            else {
                break;
            }

        }

    }
    while (true);

    return input_file;
}

boost::filesystem::path builder::upsweep(boost::filesystem::path input_file,
                                         uint16_t start_stage,
                                         reduction_strategy const *reduction_strategy,
                                         normal_computation_strategy const *normal_comp_strategy,
                                         radius_computation_strategy const *radius_comp_strategy) const
{
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
                desc_.resample,
                desc_.recompute_leaf_normals,
                desc_.recompute_leaf_radii);

    auto bvhu_file = add_to_path(base_path_, ".bvhu");
    bvh.serialize_tree_to_file(bvhu_file.string(), true);

    if ((!desc_.keep_intermediate_files) && (start_stage < 2)) {
        std::remove(input_file.string().c_str());
    }

    input_file = bvhu_file;
// LOGGER_DEBUG("Used memory: " << GetProcessUsedMemory() / 1024 / 1024 << " MiB");
    return input_file;
}

bool builder::resample_surfels(boost::filesystem::path const &input_file) const
{
    std::cout << std::endl;
    std::cout << "--------------------------------" << std::endl;
    std::cout << "resample" << std::endl;
    std::cout << "--------------------------------" << std::endl;
    LOGGER_TRACE("resample stage");

    lamure::pre::bvh bvh(memory_limit_, desc_.buffer_size, desc_.rep_radius_algo);

    if (!bvh.load_tree(input_file.string())) {
        return false;
    }

    if (bvh.state() != bvh::state_type::after_downsweep) {
        LOGGER_ERROR("Wrong processing state!");
        return false;
    }

    CPU_TIMER;
    // perform resample
    bvh.resample();

    //write resampled leaf level out
    format_xyz format_out;
    std::unique_ptr<format_xyz> dummy_format_in{new format_xyz()};
    auto xyz_res_file = add_to_path(base_path_, "_res.xyz");
    converter conv(*dummy_format_in, format_out, desc_.buffer_size);
    surfel_vector resampled_ll = bvh.get_resampled_leaf_lv_surfels();
    conv.write_in_core_surfels_out(resampled_ll, xyz_res_file.string());

    std::remove(input_file.string().c_str());

// LOGGER_DEBUG("Used memory: " << GetProcessUsedMemory() / 1024 / 1024 << " MiB");
    return true;
}

bool builder::reserialize(boost::filesystem::path const &input_file, uint16_t start_stage) const
{
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
    auto prov_file = add_to_path(base_path_, ".prov");
    auto kdn_file = add_to_path(base_path_, ".bvh");
    auto json_file = add_to_path(base_path_, ".json");

    if (bvh.nodes()[0].has_provenance()) {
      std::cout << "write paradata json description: " << json_file << std::endl;
      prov_data::write_json(json_file.string());
    }

    std::cout << "serialize surfels to file" << std::endl;
    bvh.serialize_surfels_to_file(lod_file.string(), prov_file.string(), desc_.buffer_size);

    std::cout << "serialize bvh to file" << std::endl << std::endl;
    bvh.serialize_tree_to_file(kdn_file.string(), false);

    if ((!desc_.keep_intermediate_files) && (start_stage < 3)) {
        std::remove(input_file.string().c_str());
        bvh.reset_nodes();

    }
    // LOGGER_DEBUG("Used memory: " << GetProcessUsedMemory() / 1024 / 1024 << " MiB");
    return true;
}

size_t builder::calculate_memory_limit() const
{

    size_t memory_budget = ((size_t)(desc_.memory_budget)) * 1024UL * 1024UL * 1024UL;
    LOGGER_INFO("Total physical memory: " << get_total_memory() / 1024.0 / 1024.0 / 1024.0 << " GiB");
    LOGGER_INFO("Memory budget: " << desc_.memory_budget << " GiB (use -m to choose memory budget in GiB)");
    if (get_total_memory() <= memory_budget) {
        LOGGER_ERROR("Not enough memory. Go buy more RAM");
        return false;
    }
    LOGGER_INFO("Precision for storing coordinates and radii: " << std::string((sizeof(real) == 8) ? "double" : "single"));
    return memory_budget;
}

bool builder::resample()
{
    auto input_file = fs::canonical(fs::path(desc_.input_file));
    const std::string input_file_type = input_file.extension().string();

    uint16_t start_stage = 0;
    if (input_file_type == ".xyz" ||
        input_file_type == ".ply" ||
        input_file_type == ".bin" ||
        input_file_type == ".xyz" ||
        input_file_type == ".xyz_all" ||
        input_file_type == ".xyz_grey" ||
        input_file_type == ".xyz_bin" ||
        input_file_type == ".xyz_cpn" ||
        input_file_type == ".ply")
        start_stage = 0;
    else if (input_file_type == ".bin" || input_file_type == ".bin_all")
        start_stage = 1;
    else {
        LOGGER_ERROR("Unknown input file format");
        return false;
    }

    // convert to binary file
    if (0 >= start_stage) {
        input_file = convert_to_binary(desc_.input_file, input_file_type);
        if (input_file.empty()) return false;
    }

    // downsweep (create bvh)
    if (3 >= start_stage) {
        input_file = downsweep(input_file, start_stage);
        if (input_file.empty()) return false;
    }

    // resample (create new xzy)
    bool resample_success = resample_surfels(input_file);
    return resample_success;
}

bool builder::
construct()
{
    uint16_t start_stage = 0;
    uint16_t final_stage = desc_.final_stage;

    auto input_file = fs::canonical(fs::path(desc_.input_file));
    const std::string input_file_type = input_file.extension().string();

    if (input_file_type == ".xyz" ||
        input_file_type == ".ply" ||
        input_file_type == ".xyz_grey" ||
        input_file_type == ".bin") {

        desc_.recompute_leaf_normals = true;
        desc_.recompute_leaf_radii = true;
    }

    if (input_file_type == ".xyz_cpn") {
      desc_.recompute_leaf_normals = true;
      desc_.recompute_leaf_radii = true;
    }

    if (input_file_type == ".xyz" ||
        input_file_type == ".xyz_all" ||
        input_file_type == ".xyz_grey" ||
        input_file_type == ".xyz_bin" ||
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
    std::unique_ptr<reduction_strategy> reduction_strategy{get_reduction_strategy(desc_.reduction_algo)};
    std::unique_ptr<normal_computation_strategy> normal_comp_strategy{get_normal_strategy(desc_.normal_computation_algo)};
    std::unique_ptr<radius_computation_strategy> radius_comp_strategy{get_radius_strategy(desc_.radius_computation_algo)};

    // convert to binary file
    if ((0 >= start_stage) && (0 <= final_stage)) {
        input_file = convert_to_binary(desc_.input_file, input_file_type);
        if (input_file.empty()) return false;
    }

    // convert prov data to binary
    if (desc_.prov_file != "") {
        auto prov_file = fs::canonical(fs::path(desc_.prov_file));
        const std::string prov_file_type = prov_file.extension().string();
        desc_.prov_file = convert_to_binary(desc_.prov_file, prov_file_type).string();
        if (prov_file.empty()) return false;
    }
    else {
        if (desc_.reduction_algo == lamure::pre::reduction_algorithm::ndc_prov) {
            //create a dummy prov_file
            std::ifstream surfel_bin_file(input_file.string().c_str(), std::ios::binary | std::ios::ate);
            uint64_t num_surfels = surfel_bin_file.tellg() / sizeof(surfel);
            surfel_bin_file.close();
            desc_.prov_file = input_file.string() + ".bin_prov";
            std::ofstream dummy_file(desc_.prov_file.c_str(), std::ios::out | std::ios::binary);
            for (uint64_t i = 0; i < num_surfels*sizeof(prov_data); ++i) {
                char zero = 0;
                dummy_file.write(&zero, sizeof(char));
            }
            dummy_file.close();
        }
    }

    // downsweep (create bvh)
    if ((3 >= start_stage) && (3 <= final_stage)) {
        input_file = downsweep(input_file, start_stage);
        if (input_file.empty()) return false;
    }

    // upsweep (create LOD)
    if ((4 >= start_stage) && (4 <= final_stage)) {
        input_file = upsweep(input_file, start_stage, reduction_strategy.get(), normal_comp_strategy.get(), radius_comp_strategy.get());
        if (input_file.empty()) return false;
    }

    // serialize to file
    if ((5 >= start_stage) && (5 <= final_stage)) {
        bool reserialize_success = reserialize(input_file, start_stage);
        if (!reserialize_success) return false;
    }
    return true;
}

} // namespace pre
} // namespace lamure
