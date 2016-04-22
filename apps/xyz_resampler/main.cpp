// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <omp.h>
#include <iostream>
#include <chrono>

#include <lamure/xyz/builder.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <lamure/xyz/io/format_ply.h>
#include <lamure/xyz/io/format_xyz.h>
#include <lamure/xyz/io/format_xyz_all.h>
#include <lamure/xyz/io/format_bin.h>
#include <lamure/xyz/io/converter.h>



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

        ("no-translate-to-origin",
         "do not translate surfels to origin. "
         "If this option is set, the surfels in the input file will not be "
         "translated to the root bounding box center.")

        ("mem-ratio,m",
         po::value<float>()->default_value(0.6, "0.6"),
         "the ratio to the total amount of physical memory available on the "
         "current system. This value denotes how much memory is allowed to "
         "use by the application")

        ("buffer-size,b",
         po::value<int>()->default_value(150),
         "buffer size in MiB")

        ("rep-radius-algo",
         po::value<std::string>()->default_value("gmean"),
         "Algorithm for computing representative surfel radius for tree nodes. Possible values:\n"
         "  amean - arithmetic mean\n"
         "  gmean - geometric mean\n"
         "  hmean - harmonic mean");

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
                "  OUTPUT: file in .xyz format\n";
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

        lamure::xyz::format_factory f;
        f[".xyz"] = &lamure::xyz::create_format_instance<lamure::xyz::format_xyz>;
        f[".xyz_all"] = &lamure::xyz::create_format_instance<lamure::xyz::format_xyzall>;
        f[".ply"] = &lamure::xyz::create_format_instance<lamure::xyz::format_ply>;
        f[".bin"] = &lamure::xyz::create_format_instance<lamure::xyz::format_bin>;

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
        lamure::xyz::format_abstract* inp = f[input_type]();
        lamure::xyz::format_abstract* out = f[output_type]();

        lamure::xyz::converter conv(*inp, *out, buffer_size);

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

        lamure::xyz::builder::descriptor desc;

        std::string rep_radius_algo = vm["rep-radius-algo"].as<std::string>();
        if (rep_radius_algo == "amean")
            desc.rep_radius_algo       = lamure::xyz::rep_radius_algorithm::arithmetic_mean;
        else if (rep_radius_algo == "gmean")
            desc.rep_radius_algo       = lamure::xyz::rep_radius_algorithm::geometric_mean;
        else if (rep_radius_algo == "hmean")
            desc.rep_radius_algo       = lamure::xyz::rep_radius_algorithm::harmonic_mean;
        else {
            std::cerr << "Unknown algorithm for computing representative surfel radius" << details_msg;
            return EXIT_FAILURE;
        }

        // manual check because typed_value doenst support check whether default is used
        if(vm["mem-ratio"].as<float>() != 0.6f) {
            std::cerr << "WARNING: \"mem-ratio\" flag deprecated" << std::endl;
        }
        desc.memory_ratio                 = std::max(vm["mem-ratio"].as<float>(), 0.05f);
        desc.buffer_size                  = buffer_size;
        desc.input_file                   = fs::canonical(input_file).string();
        desc.working_directory            = fs::canonical(wd).string();
        desc.max_fan_factor               = 2;
        desc.surfels_per_node             = 1024;
        desc.translate_to_origin          = !vm.count("no-translate-to-origin");
        desc.resample                     = true;
        desc.outlier_ratio                = 0.0f;
        // preprocess
        lamure::xyz::builder builder(desc);
        if (!builder.resample())
            return EXIT_FAILURE;
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Resampling total time in s: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << std::endl;

    return EXIT_SUCCESS;
}

