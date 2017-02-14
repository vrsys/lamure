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

#include <lamure/pvs/grid_regular.h>
#include <lamure/pvs/grid_regular_compressed.h>
#include <lamure/pvs/grid_octree.h>
#include <lamure/pvs/grid_octree_hierarchical.h>
#include <lamure/pvs/grid_octree_hierarchical_v2.h>
#include <lamure/pvs/grid_octree_hierarchical_v3.h>

#include <lamure/pvs/grid_optimizer_octree.h>
#include <lamure/pvs/grid_optimizer_octree_hierarchical.h>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

int main(int argc, char** argv)
{
    // Read additional data from input parameters.
    std::string pvs_input_file_path = "";
    std::string pvs_output_file_path = "";
    std::string grid_type = "";
    float optimization_threshold = 1.0f;

    namespace po = boost::program_options;
    namespace fs = boost::filesystem;

    const std::string exec_name = (argc > 0) ? fs::basename(argv[0]) : "";

    putenv((char *)"__GL_SYNC_TO_VBLANK=0");

    std::string resource_file_path = "";

    po::options_description desc("Usage: " + exec_name + " [OPTION]... INPUT\n\n"
                               "Allowed Options");
    desc.add_options()
      ("pvs-file", po::value<std::string>(&pvs_input_file_path), "specify input file of calculated pvs data (.pvs)")
      ("output-file", po::value<std::string>(&pvs_output_file_path), "specify output file of converted visibility data (.pvs)")
      ("gridtype", po::value<std::string>(&grid_type)->default_value("octree"), "specify type of grid to store visibility data")
      ("optithresh", po::value<float>(&optimization_threshold)->default_value(1.0f), "specify the threshold at which common data are converged. Default is 1.0, which means data must be 100 percent equal.");
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

    if(pvs_output_file_path == "")
    {
        std::cout << "Please specifiy PVS output file path.\n" << desc;
        return 0;
    }

    // Load pvs and grid data.
    std::cout << "Start loading grid and visibility data..." << std::endl;

    std::string grid_input_file_path = pvs_input_file_path;
    grid_input_file_path.resize(grid_input_file_path.length() - 3);
    grid_input_file_path += "grid";

    // Read the input grid file.
    lamure::pvs::grid* input_grid = lamure::pvs::pvs_database::get_instance()->load_grid_from_file(grid_input_file_path);

    if(input_grid == nullptr)
    {
        std::cout << "Error loading input grid: " << grid_input_file_path << std::endl;
        return 0;
    }

    if(input_grid->get_grid_type() != lamure::pvs::grid_regular::get_grid_identifier() && 
        input_grid->get_grid_type() != lamure::pvs::grid_regular_compressed::get_grid_identifier() && 
        input_grid->get_grid_type() != lamure::pvs::grid_octree_hierarchical_v3::get_grid_identifier())
    {
        std::cout << "Input grid must be of regular, regular compressed or hierarchical v3 grid type." << std::endl;
        return 0;
    }

    std::cout << "Detected input grid of type '" << input_grid->get_grid_type() << "'." << std::endl; 

    // Start conversion into different grid type.
    std::cout << "Preparing new grid..." << std::endl;

    lamure::pvs::grid* output_grid = nullptr;

    std::vector<lamure::node_t> ids;
    for(size_t model_index = 0; model_index < input_grid->get_num_models(); ++model_index)
    {
        ids.push_back(input_grid->get_num_nodes(model_index));
    }

    // TODO: check if the input grid uses proper amount of cells to convert into octree.
    unsigned int cells_per_axis = (unsigned int)std::round(std::pow(input_grid->get_cell_count(), 1.0/3.0));
    unsigned int depth = (unsigned int)std::round(std::sqrt(cells_per_axis)) + 1;

    size_t num_cells = depth;
    if(grid_type == lamure::pvs::grid_regular::get_grid_identifier() || 
        grid_type == lamure::pvs::grid_regular_compressed::get_grid_identifier())
    {
        num_cells = cells_per_axis;
    }

    output_grid = lamure::pvs::pvs_database::get_instance()->create_grid_by_type(grid_type, num_cells, input_grid->get_size().x, input_grid->get_position_center(), ids);

    if(output_grid == nullptr)
    {
        std::cout << "Invalid output grid type.\n" << desc;
        return 0;
    }

    std::cout << "Finished preparations." << std::endl;

    // Copy visibility data.
    std::cout << "Start copying visibility data to new grid..." << std::endl;
    
    for(size_t cell_index = 0; cell_index < input_grid->get_cell_count(); ++cell_index)
    {
        // Regular grid has different order than octree, so the proper cell at same position must be found.
        size_t input_cell_index = 0;
        const lamure::pvs::view_cell* input_cell = input_grid->get_cell_at_position(output_grid->get_cell_at_index(cell_index)->get_position_center(), &input_cell_index);
        input_grid->load_cell_visibility_from_file(pvs_input_file_path, input_cell_index);

        for(lamure::model_t model_index = 0; model_index < ids.size(); ++model_index)
        {
            for(lamure::node_t node_index = 0; node_index < ids[model_index]; ++node_index)
            {
                output_grid->set_cell_visibility(cell_index, model_index, node_index, input_cell->get_visibility(model_index, node_index));
            }
        }

        input_grid->clear_cell_visibility(input_cell_index);

        double current_percentage_done = ((double)cell_index / (double)(input_grid->get_cell_count() - 1)) * 100.0;
        std::cout << "\rcopy in progress [" << current_percentage_done << "]       " << std::flush;
    }

    std::cout << "\nFinished copy." << std::endl;

    // Optimize newly created grid.
    std::cout << "Start grid optimization..." << std::endl;

    if(grid_type == lamure::pvs::grid_octree::get_grid_identifier())
    {
        lamure::pvs::grid_optimizer_octree optimizer;
        optimizer.optimize_grid(output_grid, optimization_threshold);
    }
    else if(grid_type == lamure::pvs::grid_octree_hierarchical::get_grid_identifier() ||
            grid_type == lamure::pvs::grid_octree_hierarchical_v2::get_grid_identifier())
    {
        lamure::pvs::grid_optimizer_octree_hierarchical optimizer;
        optimizer.optimize_grid(output_grid, optimization_threshold);
    }
    
    std::cout << "Finished grid optimization." << std::endl;

    // Save newly created grid.
    std::string grid_output_file_path = pvs_output_file_path;
    grid_output_file_path.resize(grid_output_file_path.length() - 3);
    grid_output_file_path += "grid";

    std::cout << "Start writing grid file..." << std::endl;
    output_grid->save_grid_to_file(grid_output_file_path);
    std::cout << "Finished writing grid file.\nStart writing pvs file..." << std::endl;
    output_grid->save_visibility_to_file(pvs_output_file_path);
    std::cout << "Finished writing pvs file." << std::endl;

    std::cout << "\nConversion successful!" << std::endl;

    return 0;
}
