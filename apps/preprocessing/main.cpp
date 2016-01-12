// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <omp.h>
#include <iostream>
#include <chrono>

#include <lamure/pre/builder.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <lamure/pre/io/format_ply.h>
#include <lamure/pre/io/format_xyz.h>
#include <lamure/pre/io/format_xyz_all.h>
#include <lamure/pre/io/format_bin.h>
#include <lamure/pre/io/converter.h>



int main(int argc, const char *argv[])
{
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    omp_set_nested(1);

    namespace po = boost::program_options;
    namespace fs = boost::filesystem;

    const std::string exec_name = (argc > 0) ? fs::basename(argv[0]) : "";

    // define command line options
    const std::string details_msg = "\nFor details use -h or --help option.\n";
    po::variables_map vm;
    po::positional_options_description pod;
    po::options_description od_hidden("hidden");
    po::options_description od_cmd("cmd");
    po::options_description od("Usage: " + exec_name + " [OPTION]... INPUT\n"
                               "       " + exec_name + " -c INPUT OUTPUT\n\n"
                               "Allowed Options");
    od.add_options()
        ("help,h",
         "print help message")

        ("working-directory,w",
         po::value<std::string>(),
         "output and intermediate files are created in this directory (default:"
         " input file directory)")

        ("final-stage,s",
         po::value<int>()->default_value(4),
         "number of the stage to stop after.\n"
         "  0 - binary file creation\n"
         "  1 - surfel + radius computation\n"
         "  2 - downsweep/tree creation\n"
         "  3 - upsweep/LOD creation\n"
         "  4 - serialization.\n"
         "make sure the final stage number is equal or higher than the start stage,"
         "which is implicitly defined by INPUT")

        ("max-fanout",
         po::value<int>()->default_value(2),
         "maximum fan-out factor for tree properties computation. "
         "Actual factor value depends on total number of surfels and desired "
         "number of surfels per node (option -d), so it might be less than "
         "provided value")
        
        ("no-translate-to-origin",
         "do not translate surfels to origin. "
         "If this option is set, the surfels in the input file will not be "
         "translated to the root bounding box center.")

        ("desired,d",
         po::value<int>()->default_value(1024),
         "the application tries to achieve a number of surfels per node close to "
         "this value")

        ("recompute,r",
         "recompute surfel normals and radii, even if they data is present in the "
         "input data")

        ("neighbours",
         po::value<int>()->default_value(40),
         "Number of neighbours for the plane-fitting when computing normals")

        ("keep-interm,k",
         "prevents deletion of intermediate files")

        ("mem-ratio,m",
         po::value<float>()->default_value(0.6, "0.6"),
         "the ratio to the total amount of physical memory available on the "
         "current system. This value denotes how much memory is allowed to "
         "use by the application")

        ("buffer-size,b",
         po::value<int>()->default_value(150),
         "buffer size in MiB")

        ("reduction-algo",
         po::value<std::string>()->default_value("ndc"),
         "Reduction strategy for the LOD construction. Possible values:\n"
         "  ndc - normal deviation clustering (ndc)\n"
         "  const - ndc with constant radius\n"
         "  everysecond - take every fanout-factor's surfel\n"
         "  random - randomly select points with possible duplicates\n"
         "  entropy - take sufels with min entropy\n"
         "  particlesim - perform particle simulation\n"
         "  hierarchical - create clusters by binary splitting of the point cloud")
         "  kclustering - hash-based k-clustering")

        ("normal-computation-algo",
         po::value<std::string>()->default_value("planefitting"),
         "Algorithm for computing surfel normal. Possible values:\n"
         "  planefitting ")

         ("radius-computation-algo",
         po::value<std::string>()->default_value("naturalneighbours"),
         "Algorithm for computing surfel radius. Possible values:\n"
         "  averagedistance \n"
         "  naturalneighbours")
 
        ("rep-radius-algo",
         po::value<std::string>()->default_value("gmean"),
         "Algorithm for computing representative surfel radius for tree nodes. Possible values:\n"
         "  amean - arithmetic mean\n"
         "  gmean - geometric mean\n"
         "  hmean - harmonic mean")

        ("convert,c",
         "convert RAW point data from one format to another (see convertion mode description below).");

    od_hidden.add_options()
        ("files,i",
         po::value<std::vector<std::string>>()->composing()->required(),
         "files");

    od_cmd.add(od_hidden).add(od);

    // parse command line options
    try {
        pod.add("files", -1);

        po::store(po::command_line_parser(argc, argv)
                  .options(od_cmd)
                  .positional(pod)
                  .run(), vm);

        if (vm.count("help")) {
            std::cout << od << std::endl;
            std::cout << "Build mode:\n"
                "  INPUT can be one with the following extensions:\n"
                "    .xyz, .xyz_all, .ply - stage 0: start from the beginning\n"
                "    .bin - stage 1: start from normal + radius computation\n"
                "    .bin_all - stage 2: start from downsweep/tree creation\n"
                "    .kdnd - stage 3: start from upsweep/LOD creation\n"
                "    .kdnu - stage 4: start from serializer\n"
                "  last two stages require intermediate files to be present in the working directory (-k option).\n"
                "Conversion mode (-c option):\n"
                "  INPUT: file in either .xyz, .xyz_all, or .ply format\n"
                "  OUTPUT: file in either .xyz_all or .bin_all format\n";
            return EXIT_SUCCESS;
        }

        po::notify(vm);
    }
    catch (po::error& e) {
        std::cerr << "Error: " << e.what() << details_msg;
        return EXIT_FAILURE;
    }

    const auto files         = vm["files"].as<std::vector<std::string>>();
    const size_t buffer_size = size_t(std::max(vm["buffer-size"].as<int>(), 20)) * 1024UL * 1024UL;

    if (vm.count("convert")) {
        // convertion mode
        if (files.size() != 2) {
            std::cerr << "Convestion mode needs one input file "
                         "and one output file to be specified" << details_msg;
            return EXIT_FAILURE;
        }
        const auto input_file = fs::canonical(files[0]);
        const auto output_file = fs::absolute(files[1]);

        lamure::pre::format_factory f;
        f[".xyz"] = &lamure::pre::create_format_instance<lamure::pre::format_xyz>;
        f[".xyz_all"] = &lamure::pre::create_format_instance<lamure::pre::format_xyzall>;
        f[".ply"] = &lamure::pre::create_format_instance<lamure::pre::format_ply>;
        f[".bin"] = &lamure::pre::create_format_instance<lamure::pre::format_bin>;

        auto input_type = input_file.extension().string();
        auto output_type = output_file.extension().string();

        if (f.find(input_type) == f.end()) {
            std::cerr << "Unknown input file format" << details_msg;
            return EXIT_FAILURE;
        }
        if (f.find(output_type) == f.end()) {
            std::cerr << "Unknown output file format" << details_msg;
            return EXIT_FAILURE;
        }
        lamure::pre::format_abstract* inp = f[input_type]();
        lamure::pre::format_abstract* out = f[output_type]();

        lamure::pre::converter conv(*inp, *out, buffer_size);

        conv.convert(input_file.string(), output_file.string());

        delete inp;
        delete out;
    }
    else {
        // build mode
        if (files.size() != 1) {
            std::cerr << "Exactly one input file must be specified" << details_msg;
            return EXIT_FAILURE;
        }

        // preconditions
        const auto input_file = fs::absolute(files[0]);
        auto wd = input_file.parent_path();

        if (vm.count("working-directory")) {
            wd = fs::absolute(fs::path(vm["working-directory"].as<std::string>()));
        }
        if (!fs::exists(input_file)) {
            std::cerr << "Input file does not exist" << std::endl;
            return EXIT_FAILURE;
        }       
        if (!fs::exists(wd)) {
            std::cerr << "Provided working directory does not exist" << std::endl;
            return EXIT_FAILURE;
        }
        if (vm["final-stage"].as<int>() < 0 || vm["final-stage"].as<int>() > 4) {
            std::cerr << "Wrong final stage value" << details_msg;
            return EXIT_FAILURE;
        }
        if (vm["desired"].as<int>() < 5) {
            std::cerr << "Too small desired number of surfels per node. "
                         "Please set a greater value with option -d" << details_msg;
            return EXIT_FAILURE;
        }

        // set building options
        lamure::pre::builder::descriptor desc;
        std::string reduction_algo = vm["reduction-algo"].as<std::string>();
        std::string normal_computation_algo = vm["normal-computation-algo"].as<std::string>();
        std::string radius_computation_algo = vm["radius-computation-algo"].as<std::string>();
        std::string rep_radius_algo = vm["rep-radius-algo"].as<std::string>();

        if (reduction_algo == "ndc")
            desc.reduction_algo        = lamure::pre::reduction_algorithm::ndc;
        else if (reduction_algo == "const")
            desc.reduction_algo        = lamure::pre::reduction_algorithm::constant;
        else if (reduction_algo == "everysecond")
            desc.reduction_algo        = lamure::pre::reduction_algorithm::every_second;
        else if (reduction_algo == "random") 
            desc.reduction_algo        = lamure::pre::reduction_algorithm::random;
        else if (reduction_algo == "entropy") 
            desc.reduction_algo        = lamure::pre::reduction_algorithm::entropy;
        else if (reduction_algo == "particlesim")
            desc.reduction_algo        = lamure::pre::reduction_algorithm::particle_sim;
        else if (reduction_algo == "hierarchical") 
            desc.reduction_algo        = lamure::pre::reduction_algorithm::hierarchical_clustering;
        else if (reduction_algo == "kclustering")
            desc.reduction_algo        = lamure::pre::reduction_algorithm::k_clustering;
        else {
            std::cerr << "Unknown reduction algorithm" << details_msg;
            return EXIT_FAILURE;
        }

        if (normal_computation_algo == "planefitting")
            desc.normal_computation_algo      = lamure::pre::normal_computation_algorithm::plane_fitting;
        else {
            std::cerr << "Unknown algorithm for computing surfel normal" << details_msg;
            return EXIT_FAILURE;
        }

        if (radius_computation_algo == "averagedistance")
            desc.radius_computation_algo      = lamure::pre::radius_computation_algorithm::average_distance;
        else if (radius_computation_algo == "naturalneighbours")
            desc.radius_computation_algo      = lamure::pre::radius_computation_algorithm::natural_neighbours;
        else {
            std::cerr << "Unknown algorithm for computing surfel radius" << details_msg;
            return EXIT_FAILURE;
        }

        if (rep_radius_algo == "amean")
            desc.rep_radius_algo       = lamure::pre::rep_radius_algorithm::arithmetic_mean;
        else if (rep_radius_algo == "gmean")
            desc.rep_radius_algo       = lamure::pre::rep_radius_algorithm::geometric_mean;
        else if (rep_radius_algo == "hmean")
            desc.rep_radius_algo       = lamure::pre::rep_radius_algorithm::harmonic_mean;
        else {
            std::cerr << "Unknown algorithm for computing representative surfel radius" << details_msg;
            return EXIT_FAILURE;
        }

        desc.input_file                = fs::canonical(input_file).string();
        desc.working_directory         = fs::canonical(wd).string();
        desc.max_fan_factor            = std::min(std::max(vm["max-fanout"].as<int>(), 2), 8);
        desc.surfels_per_node          = vm["desired"].as<int>();
        desc.final_stage               = vm["final-stage"].as<int>();
        desc.compute_normals_and_radii = vm.count("recompute");
        desc.keep_intermediate_files   = vm.count("keep-interm");
        desc.memory_ratio              = std::max(vm["mem-ratio"].as<float>(), 0.05f);
        desc.buffer_size               = buffer_size;
        desc.number_of_neighbours      = std::max(vm["neighbours"].as<int>(), 1);
        desc.translate_to_origin       = !vm.count("no-translate-to-origin");

        // preprocess
        lamure::pre::builder builder(desc);
        if (!builder.construct())
            return EXIT_FAILURE;
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Preprocessing total time in s: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << std::endl;

    return EXIT_SUCCESS;
}

