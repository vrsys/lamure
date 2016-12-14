// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <algorithm>
#include <string>
#include <vector>
#include <iostream>

#include <lamure/pvs/pvs_database.h>
#include <lamure/pvs/pvs_utils.h>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

int main(int argc, char** argv)
{
    // Read additional data from input parameters.
    std::string pvs_input_file_path = "";
    unsigned int num_steps = 11;

    namespace po = boost::program_options;
    namespace fs = boost::filesystem;

    const std::string exec_name = (argc > 0) ? fs::basename(argv[0]) : "";

    putenv((char *)"__GL_SYNC_TO_VBLANK=0");

    std::string resource_file_path = "";

    po::options_description desc("Usage: " + exec_name + " [OPTION]... INPUT\n\n"
                               "Allowed Options");
    desc.add_options()
      ("pvs-file,p", po::value<std::string>(&pvs_input_file_path), "specify input file of calculated pvs data")
      ("numsteps,n", po::value<unsigned int>(&num_steps)->default_value(11), "specify the number of intervals the occlusion values will be split into");
      ;

    po::variables_map vm;
    auto parsed_options = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
    po::store(parsed_options, vm);
    po::notify(vm);

    if(pvs_input_file_path == "")
    {
        std::cout << "Please specifiy PVS input file path.\n" << desc;
        return 0;
    }

    // Load pvs and grid data.
    std::string pvs_grid_input_file_path = pvs_input_file_path;
    std::string visibility_output_path = pvs_input_file_path;

    if(pvs_grid_input_file_path != "")
    {
        pvs_grid_input_file_path.resize(pvs_grid_input_file_path.length() - 3);
        visibility_output_path.resize(visibility_output_path.length() - 4);

        pvs_grid_input_file_path += "grid";
        visibility_output_path += "_visibility.txt";

        std::cout << "visibility data output file:\n" << visibility_output_path << std::endl;
    }
    else
    {
        std::cout << "Input .pvs file required." << std::endl;
        return -1;
    }

    lamure::pvs::pvs_database* pvs_db = lamure::pvs::pvs_database::get_instance();
    if(!pvs_db->load_pvs_from_file(pvs_grid_input_file_path, pvs_input_file_path))
    {
        std::cout << "not able to load pvs from files " << pvs_input_file_path  << " and " << pvs_grid_input_file_path << std::endl; 
        return -1;
    }

    lamure::pvs::analyze_grid_visibility(pvs_db->get_visibility_grid(), num_steps, visibility_output_path);
    std::cout << "done" << std::endl;

    return 0;
}
