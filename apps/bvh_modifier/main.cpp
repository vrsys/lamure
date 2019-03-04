// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/node_serializer.h>
#include <lamure/pre/serialized_surfel.h>
#include <lamure/utils.h>

#include <omp.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <scm/gl_core/math/math.h>
#include <iostream>

#include "tree_modifier.h"

lamure::mat4r loadMatrix(const std::string& filename)
{
    lamure::mat4r mat = lamure::mat4r::identity();
    if (filename == "eye")
        return mat;

    std::ifstream f(filename);
    if (!f.is_open()) {
        std::cerr << "Unable to open transformation file: \"" 
                  << filename << "\". Exiting...\n";
        exit(EXIT_FAILURE);
    }
    else {
        std::string matrix_values_string;
        std::getline(f, matrix_values_string);
        std::stringstream sstr(matrix_values_string);
        for (int i = 0; i < 16; ++i)
            sstr >> mat[i];
    }
    return scm::math::transpose(mat);
}

int main(int argc, const char *argv[])
{

  scm::math::vec3f extend_bbx_max = scm::math::vec3f(0.0,0.0,0.0);
  scm::math::vec3f extend_bbx_min = scm::math::vec3f(0.0,0.0,0.0);

    omp_set_nested(1);

    namespace po = boost::program_options;
    namespace fs = boost::filesystem;

    const std::string exec_name = (argc > 0) ? fs::path(argv[0]).filename().string() : "";

    // parse command line options
    po::variables_map vm;
    try {
        po::options_description od("Usage: " + exec_name + " [OPTION]... KDN-FILE...\n\nAllowed Options");
        od.add_options()
            ("help,h",
             "print help message")

            ("mode,m",
             po::value<std::string>()->required(),
             "mode of operation. Available modes:\n"
             "  info\n"
             "  complement - first input is the model to be cleaned; the second model defines cleaning volume\n"
             "  hist - first input is the reference model, others models are to be adjusted\n"
             "  radius - multiply surfels' radii with a value given by option -r")

            ("input-files,i",
             po::value<std::vector<std::string>>()->composing()->required(),
             "input files in .bvh format")

            ("transforms,t",
             po::value<std::vector<std::string>>()->composing(),
             "transform files for the models (HINT: use 'eye' keyword for identity matrix)")

            ("relax-levels,l",
             po::value<int>()->default_value(0),
             "How many bvh levels to step up from the leaf level in the second "
             "model to perform intersection test")

            ("radius,r",
             po::value<float>(),
             "Radius multiplier")

            ("extend,e",
             po::value<std::vector<float>>()->multitoken(),
             "Extend bounding boxes by values x y z -x -y -z, e.g. 0.01 0.0 0.0 0.0 0.0 0.0 to extend positive x axis by 1cm. Please note that all values have to be positive.");


        po::positional_options_description pod;
        pod.add("input-files", -1);

        po::store(po::command_line_parser(argc, argv).options(od).positional(pod).run(), vm);

        if (vm.count("help")) {
            std::cout << od << std::endl;
            std::cout << "THIS APPLICATION MODIFES DATA IN .LOD FILES WITHOUT POSSIBILITY TO UNDO!\n"
                         "PLEASE BACKUP YOUR .LOD FILES TO PREVENT LOSS OF INFORMATION\n";
            return EXIT_SUCCESS;
        }
        po::notify(vm);
    }
    catch(po::error& e) {
        std::cerr << "Error: " << e.what() << std::endl
                  << "For details use " << argv[0] << " -h or --help option." << std::endl;
        return EXIT_FAILURE;
    }

    // get input filenames
    auto inp_files = vm["input-files"].as<std::vector<std::string>>();

    if(vm.count("extend")){
      auto bbx_extend_values = vm["extend"].as<std::vector<float>>();
      if(6 != bbx_extend_values.size()){
        std::cerr << argv[0] << std::endl << "Error: too few values for option -extend given." << std::endl
                  << "For details use " << argv[0] << " -h or --help option." << std::endl;
        return EXIT_FAILURE;
      }
      bool all_positive = true;
      for(const auto& v : bbx_extend_values){
	if(v < 0.0){
	  all_positive = false;
	}
      }
      if(!all_positive){
        std::cerr << argv[0] << std::endl << "Error: all values for option -extend ahve to be positive." << std::endl
                  << "For details use " << argv[0] << " -h or --help option." << std::endl;
        return EXIT_FAILURE;	
      }
      // apply values for bbx_extend from cmdline
      extend_bbx_max = scm::math::vec3f(bbx_extend_values[0],bbx_extend_values[1],bbx_extend_values[2]);
      extend_bbx_min = scm::math::vec3f(bbx_extend_values[3],bbx_extend_values[4],bbx_extend_values[5]);
    }


    std::vector<std::string> inp_transforms;
    if (vm.count("transforms")) 
        inp_transforms = vm["transforms"].as<std::vector<std::string>>();


    collision_detector::objectArray bvhs;

    auto trans_it = inp_transforms.begin();

    for (const auto& s : inp_files) {
        auto tr = std::make_shared<lamure::pre::bvh>(0, 0);
        tr->load_tree(s);
        if (tr->state() < lamure::pre::bvh::state_type::serialized) {
            std::cerr << "file of wrong processing state: " << lamure::pre::bvh::state_to_string(tr->state()) << std::endl;
            return EXIT_FAILURE;
        }
        tr->print_tree_properties();
        lamure::mat4r mat = lamure::mat4r::identity();
        
        if (trans_it != inp_transforms.end()) {
            mat = loadMatrix(*trans_it);
            ++trans_it;
        }

        bvhs.push_back(std::make_pair(tr, mat));
    }
    
    for (const auto& t : bvhs) {
        auto trans = scm::math::transpose(t.second);
        std::cout << "transform: ";
        for (int i = 0; i < 16; ++i)
            std::cout << trans[i] << " ";
        std::cout << std::endl;
    }

    // relative complement
    if (vm["mode"].as<std::string>() == "complement") {
        if (inp_files.size() != 2) {
            std::cerr << "Exactly two input files must be specified" << std::endl;
            return EXIT_FAILURE;
        }
        TreeModifier tm(bvhs);
        tm.complementOnFirstTree(vm["relax-levels"].as<int>(), extend_bbx_max, extend_bbx_min);
    }

    // histogram matching
    if (vm["mode"].as<std::string>() == "hist") {
        if (inp_files.size() < 2) {
            std::cerr << "At least two input files must be specified" << std::endl;
            return EXIT_FAILURE;
        }
        TreeModifier tm(bvhs);
        tm.histogrammatchSecondTree(extend_bbx_max, extend_bbx_min);
    }
    
    // radius multiplier
    if (vm["mode"].as<std::string>() == "radius") {
        if (!vm.count("radius")) {
            std::cerr << "Option -r is required in this mode" << std::endl;
            return EXIT_FAILURE;
        }
        TreeModifier tm(bvhs);
        tm.MultRadii(vm["radius"].as<float>());
    }

    // bvh information
    if (vm["mode"].as<std::string>() == "info") {
        for (const auto& t : bvhs) {
            for (uint32_t j = 0; j <= t.first->depth(); ++j) {
                auto range = t.first->get_node_ranges(j);

                for (size_t i = range.first; i < range.first + 4 && i < range.first + range.second; ++i) {
                    //std::cout << i << " centroid:   " << t.first->nodes()[i].centroid() << std::endl;
                    std::cout << i << " rep radius: " << t.first->nodes()[i].avg_surfel_radius() << std::endl;
                }
            } // */

            /*
            using namespace lamure;
            pre::node_serializer ser(t.first->max_surfels_per_node(), 0);
            ser.open(add_to_path(t.first->base_path(), ".lod").string(), true);

            pre::surfel::surfel_vector surfels;
            ser.read_node_immediate(surfels, 32767);

            for(const auto& s: surfels)
                    std::cout << s.radius() << std::endl;
                    // */

        }
    }

    return EXIT_SUCCESS;
}

