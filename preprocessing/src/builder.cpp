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
#include <lamure/pre/reduction_normal_deviation_clustering.h>
#include <lamure/pre/reduction_constant.h>
#include <lamure/pre/reduction_every_second.h>
#include <lamure/pre/reduction_random.h>

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
    else if (input_file_type == ".bvhd")
        start_stage = 3;
    else if (input_file_type == ".bvhu")
        start_stage = 4;
    else {
        LOGGER_ERROR("Unknown input file format");
        return false;
    }

    // init algorithms
    reduction_strategy *reduction_strategy;
    switch (desc_.reduction_algo) {
        case reduction_algorithm::ndc:
            reduction_strategy = new reduction_normal_deviation_clustering();
            break;
        case reduction_algorithm::constant:
            reduction_strategy = new reduction_constant();
            break;
        case reduction_algorithm::every_second:
            reduction_strategy = new reduction_every_second();
            break;
        case reduction_algorithm::random:
            reduction_strategy = new reduction_random();
            break;          
        default:
            LOGGER_ERROR("Non-implemented reduction algorithm");
            return false;
    };

    normal_computation_strategy *normal_comp_strategy;
    switch (desc_.normal_computation_algo) {
        case normal_computation_algorithm::plane_fitting:
            normal_comp_strategy = new normal_computation_plane_fitting();
            break;       
        default:
            LOGGER_ERROR("Non-implemented normal computation algorithm");
            return false;
    };


    radius_computation_strategy *radius_comp_strategy;
    switch (desc_.radius_computation_algo) {
        case radius_computation_algorithm::average_distance:
            radius_comp_strategy = new radiusComputationAverageDistance();
            break;
        default:
            LOGGER_ERROR("Non-implemented radius computation algorithm");
            return false;
    };


    // convert to binary file
    if ((0 >= start_stage) && (0 <= final_stage)) {
        std::cout << "--------------------------------" << std::endl;
        std::cout << "convert input file" << std::endl;
        std::cout << "--------------------------------" << std::endl;

        LOGGER_TRACE("convert to a binary file");

        format_abstract* format_in;
        auto binary_file = base_path_;

        if (input_file_type == ".xyz") {
            binary_file += ".bin";
            format_in = new format_xyz();
        }
        else if (input_file_type == ".xyz_all") {
            binary_file += ".bin_all";
            format_in = new format_xyzall();
        }
        else if (input_file_type == ".ply") {
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

        input_file = binary_file;

        LOGGER_DEBUG("Used memory: " << GetProcessUsedMemory() / 1024 / 1024 << " MiB");
    }

    // (re)compute normals and radii
    if ((1 >= start_stage) && (1 <= final_stage) && desc_.compute_normals_and_radii) {
        std::cout << "--------------------------------" << std::endl;
        std::cout << "compute normals and radii" << std::endl;
        std::cout << "--------------------------------" << std::endl;

        const std::string input_file_type = input_file.extension().string();

        // rename input file if already in .bin_all format
        if (input_file_type == ".bin_all")
        {
            auto orig_file = input_file.parent_path() / (input_file.stem().string() + "_orig" + input_file.extension().string());
            fs::rename(input_file, orig_file);
            input_file = orig_file;
        }

        // create kd-bvh
        LOGGER_TRACE("create kd-bvh");

        lamure::pre::bvh bvh(memory_limit_, desc_.buffer_size, desc_.rep_radius_algo);

        bvh.init_tree(input_file.string(), 2, 1000, base_path_);
        bvh.print_tree_properties();

        bvh.downsweep(false, input_file.string(), true);

        // compute normals and radii
        LOGGER_TRACE("compute normals and radii");

        CPU_TIMER;
        bvh.compute_normals_and_radii(desc_.number_of_neighbours);

        // set new input file name
        //fs::path input_file_path = fs::path(input_file);
        input_file = add_to_path(base_path_, ".bin_all");
    }

    // downsweep (create bvh)
    if ((2 >= start_stage) && (2 <= final_stage)) {
        std::cout << "--------------------------------" << std::endl;
        std::cout << "bvh properties" << std::endl;
        std::cout << "--------------------------------" << std::endl;

        lamure::pre::bvh bvh(memory_limit_, desc_.buffer_size, desc_.rep_radius_algo);

        bvh.init_tree(input_file.string(),
                          desc_.max_fan_factor,
                          desc_.surfels_per_node,
                          base_path_);

        bvh.print_tree_properties();
        std::cout << std::endl;

        std::cout << "--------------------------------" << std::endl;
        std::cout << "downsweep" << std::endl;
        std::cout << "--------------------------------" << std::endl;
        LOGGER_TRACE("downsweep stage");

        CPU_TIMER;
        bvh.downsweep(desc_.translate_to_origin, input_file.string());

        auto bvhd_file = add_to_path(base_path_, ".bvhd");

        bvh.serialize_tree_to_file(bvhd_file.string(), true);

        if ((!desc_.keep_intermediate_files) && (start_stage < 1))
        {
            // do not remove input file
            //std::remove(input_file.string().c_str());
        }

        input_file = bvhd_file;

        LOGGER_DEBUG("Used memory: " << GetProcessUsedMemory() / 1024 / 1024 << " MiB");
    }

    // upsweep (create LOD)
    if ((3 >= start_stage) && (3 <= final_stage)) {
        std::cout << "--------------------------------" << std::endl;
        std::cout << "upsweep" << std::endl;
        std::cout << "--------------------------------" << std::endl;
        LOGGER_TRACE("upsweep stage");

        lamure::pre::bvh bvh(memory_limit_, desc_.buffer_size, desc_.rep_radius_algo);
        
        if (!bvh.load_tree(input_file.string())) {
            return false;
        }

        if (bvh.state() != bvh::state_type::after_downsweep) {
            LOGGER_ERROR("Wrong processing state!");
            return false;
        }

        CPU_TIMER;


        // perform upsweep
        bvh.upsweep(*reduction_strategy);
        delete reduction_strategy;

        auto bvhu_file = add_to_path(base_path_, ".bvhu");
        bvh.serialize_tree_to_file(bvhu_file.string(), true);

        if ((!desc_.keep_intermediate_files) && (start_stage < 2)) {
            std::remove(input_file.string().c_str());
        }

        input_file = bvhu_file;
        LOGGER_DEBUG("Used memory: " << GetProcessUsedMemory() / 1024 / 1024 << " MiB");
    }

    // serialize to file
    if ((4 >= start_stage) && (4 <= final_stage)) {
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
        LOGGER_DEBUG("Used memory: " << GetProcessUsedMemory() / 1024 / 1024 << " MiB");
    }
    return true;
}

} // namespace pre
} // namespace lamure
