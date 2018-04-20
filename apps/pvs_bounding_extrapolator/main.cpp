// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <algorithm>
#include <string>
#include <vector>
#include <iostream>

#include <lamure/pvs/grid.h>
#include <lamure/pvs/pvs_database.h>
#include <lamure/pvs/pvs_bounds_extrapolator_from_outer_cells.h>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

int main(int argc, char** argv)
{
    // Read additional data from input parameters.
    std::string pvs_input_file_path = "";

    namespace po = boost::program_options;
    namespace fs = boost::filesystem;

    const std::string exec_name = (argc > 0) ? fs::basename(argv[0]) : "";

    putenv((char *)"__GL_SYNC_TO_VBLANK=0");

    std::string resource_file_path = "";

    po::options_description desc("Usage: " + exec_name + " [OPTION]... INPUT\n\n"
                               "Allowed Options");
    desc.add_options()
      ("pvs-file", po::value<std::string>(&pvs_input_file_path), "specify input file of calculated pvs data (.pvs)");
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
    std::cout << "Start loading grid and visibility data..." << std::endl;

    std::string grid_input_file_path = pvs_input_file_path;
    grid_input_file_path.resize(grid_input_file_path.length() - 3);
    grid_input_file_path += "grid";

    // Read the input grid file.
    lamure::pvs::grid* input_grid = lamure::pvs::pvs_database::get_instance()->load_grid_from_file(grid_input_file_path, pvs_input_file_path);

    if(input_grid == nullptr)
    {
        std::cout << "Error loading input grid: " << grid_input_file_path << std::endl;
        return 0;
    }

    std::cout << "Loaded input grid of type '" << input_grid->get_grid_type() << "'." << std::endl;

    // Calculate grid to expand the visibility of the input grid.
    std::cout << "Extrapolating bounding visibility ..." << std::endl;
    lamure::pvs::pvs_bounds_extrapolator_from_outer_cells extrapolator;
    lamure::pvs::grid* bounding_grid = extrapolator.extrapolate_from_grid(input_grid);

    // Save bounding grid.
    std::string bounding_visibility_output_file_path = pvs_input_file_path;
    bounding_visibility_output_file_path.resize(bounding_visibility_output_file_path.length() - 4);
    bounding_visibility_output_file_path += "_bounding.pvs";

    std::cout << "Start writing pvs file..." << std::endl;
    bounding_grid->save_visibility_to_file(bounding_visibility_output_file_path);
    std::cout << "Finished writing pvs file." << std::endl;

    std::cout << "\nBounding visibility created successfully!" << std::endl;

    return 0;
}
