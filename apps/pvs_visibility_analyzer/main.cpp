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
#include <fstream>

#include <lamure/pvs/pvs_database.h>
#include <lamure/pvs/pvs_utils.h>
#include <lamure/pvs/view_cell_regular.h>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

void compare_grid_visibility(std::string visibility_path_one, std::string visibility_path_two, unsigned int num_steps);

int main(int argc, char** argv)
{
    // Read additional data from input parameters.
    std::string pvs_input_file_path = "";
    std::string second_pvs_input_file_path = "";
    unsigned int num_steps = 11;

    namespace po = boost::program_options;
    namespace fs = boost::filesystem;

    const std::string exec_name = (argc > 0) ? fs::basename(argv[0]) : "";

    putenv((char *)"__GL_SYNC_TO_VBLANK=0");

    std::string resource_file_path = "";

    po::options_description desc("Usage: " + exec_name + " [OPTION]... INPUT\n\n"
                               "Allowed Options");
    desc.add_options()
      ("pvs-file,p", po::value<std::string>(&pvs_input_file_path), "specify input file of calculated pvs data (.pvs)")
      ("2nd-pvs-file", po::value<std::string>(&second_pvs_input_file_path), "specify input file of calculated pvs data (.pvs)")
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

    // First case: only one data set given, so analyze visibility of the given data set.
    if(second_pvs_input_file_path == "")
    {
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

        if(!pvs_db->load_pvs_from_file(pvs_grid_input_file_path, pvs_input_file_path, true))
        {
            std::cout << "not able to load pvs from files " << pvs_input_file_path  << " and " << pvs_grid_input_file_path << std::endl; 
            return -1;
        }

        lamure::pvs::analyze_grid_visibility(pvs_db->get_visibility_grid(), num_steps, visibility_output_path);
    }
    else
    {
        // Two data sets given. Check difference between data sets.
        compare_grid_visibility(pvs_input_file_path, second_pvs_input_file_path, num_steps);
    }

    std::cout << "done" << std::endl;

    return 0;
}

void compare_grid_visibility(std::string visibility_path_one, std::string visibility_path_two, unsigned int num_steps)
{
    std::vector<size_t> difference_percent_interval_counter;
    difference_percent_interval_counter.resize(num_steps);

    std::cout << "Loading visibility data of input grids..." << std::endl;

    // Load pvs and grid data.
    std::string pvs_grid_input_file_path = visibility_path_one;
    std::string second_pvs_grid_input_file_path = visibility_path_two;

    pvs_grid_input_file_path.resize(pvs_grid_input_file_path.length() - 3);
    pvs_grid_input_file_path += "grid";

    second_pvs_grid_input_file_path.resize(second_pvs_grid_input_file_path.length() - 3);
    second_pvs_grid_input_file_path += "grid";

    const lamure::pvs::grid* grid_one = lamure::pvs::pvs_database::get_instance()->load_grid_from_file(pvs_grid_input_file_path, visibility_path_one);
    const lamure::pvs::grid* grid_two = lamure::pvs::pvs_database::get_instance()->load_grid_from_file(second_pvs_grid_input_file_path, visibility_path_two);

    if(grid_one->get_cell_count() != grid_two->get_cell_count())
    {
        std::cout << "Grids to compare have unequal number of cells." << std::endl;
        return;
    }

    std::cout << "Visibility data loaded, starting comparison..." << std::endl;

    size_t total_num_visible_nodes_common = 0;
    size_t total_num_visible_nodes_one = 0;
    size_t total_num_visible_nodes_two = 0;

    size_t total_num_nodes = 0;

    double highest_visibility_difference = 0.0;
    double lowest_visibility_difference = 100.0;

    std::string output_file_name = visibility_path_one;
    output_file_name.resize(output_file_name.length() - 4);
    output_file_name += "_visibility_comparison.txt";

    std::ofstream file_out;
    file_out.open(output_file_name);

    if(!file_out.is_open())
    {
        std::cout << "Not able to create output file. Visibility comparison aborted." << std::endl;
        std::cout << "Theoretic path: " << output_file_name << std::endl;
        return;
    }

    // Iterate over view cells and models to colect visibility data.
    for(size_t cell_index = 0; cell_index < grid_one->get_cell_count(); ++cell_index)
    {
        size_t cell_num_visible_nodes_common = 0;
        size_t cell_num_visible_nodes_one = 0;
        size_t cell_num_visible_nodes_two = 0;

        size_t cell_num_nodes = 0;

        const lamure::pvs::view_cell_regular* view_cell_one = (const lamure::pvs::view_cell_regular*)grid_one->get_cell_at_index(cell_index);
        const lamure::pvs::view_cell_regular* view_cell_two = (const lamure::pvs::view_cell_regular*)grid_two->get_cell_at_index(cell_index);

        for(size_t model_index = 0; model_index < grid_one->get_num_models(); ++model_index)
        {
            cell_num_visible_nodes_common += (view_cell_one->get_bitset(model_index) & view_cell_two->get_bitset(model_index)).count();
            cell_num_visible_nodes_one += view_cell_one->get_bitset(model_index).count();
            cell_num_visible_nodes_two += view_cell_two->get_bitset(model_index).count();

            cell_num_nodes += grid_one->get_num_nodes(model_index);
        }

        // Calculate visibilities.
        double cell_visibility_one = ((double)cell_num_visible_nodes_one / (double)cell_num_nodes) * 100.0;
        double cell_visibility_two = ((double)cell_num_visible_nodes_two / (double)cell_num_nodes) * 100.0;
        double cell_visibility_common = ((double)cell_num_visible_nodes_common / (double)cell_num_nodes) * 100.0;
        double cell_visibility_difference = std::abs(cell_visibility_one - cell_visibility_two);
        
        // Check for highest or lowest visibility in grid.
        if(cell_visibility_difference > highest_visibility_difference)
        {
            highest_visibility_difference = cell_visibility_difference;
        }
        if(cell_visibility_difference < lowest_visibility_difference)
        {
            lowest_visibility_difference = cell_visibility_difference;
        }

        // Put visibility difference into proper difference interval.
        double interval_size = 100.0 / (double)num_steps;
        int difference_index = std::round((cell_visibility_difference - interval_size * 0.5) / interval_size);
        difference_index = std::max(0, std::min(difference_index, (int)(num_steps - 1)));

        difference_percent_interval_counter[difference_index] += 1;

        // Update total nodes.
        total_num_visible_nodes_common += cell_num_visible_nodes_common;
        total_num_visible_nodes_one += cell_num_visible_nodes_one;
        total_num_visible_nodes_two += cell_num_visible_nodes_two;

        total_num_nodes += cell_num_nodes;

        file_out << "view cell " << cell_index << " visibility grid one: " << cell_visibility_one << std::endl;
        file_out << "view cell " << cell_index << " visibility grid two: " << cell_visibility_two << std::endl;
        file_out << "view cell " << cell_index << " visibility common: " << cell_visibility_common << std::endl;

        std::cout << "\rcells processed: " << (cell_index + 1) << "/" << grid_one->get_cell_count();
    }
    std::cout << std::endl;

    // Calculate some interesting values from the collected data.
    double visibility_one = ((double)total_num_visible_nodes_one / (double)total_num_nodes) * 100.0;
    double visibility_two = ((double)total_num_visible_nodes_two / (double)total_num_nodes) * 100.0;

    double visibility_common = ((double)total_num_visible_nodes_common / (double)total_num_nodes) * 100.0;

    double visibility_one_only = visibility_one - visibility_common;
    double visibility_two_only = visibility_two - visibility_common;

    file_out << "\nvisibility grid one: " << visibility_one << std::endl;
    file_out << "visibility grid two: " << visibility_two << std::endl;
    file_out << "visibility common: " << visibility_common << std::endl;
    file_out << "\nvisible only by one: " << visibility_one_only << std::endl;
    file_out << "visible only by two: " << visibility_two_only << std::endl;
    file_out << "\nhighest difference in cell: " << highest_visibility_difference << std::endl;
    file_out << "lowest difference in cell: " << lowest_visibility_difference << std::endl;

    size_t num_cells = grid_one->get_cell_count();
    file_out << std::endl;

    for(size_t difference_index = 0; difference_index < difference_percent_interval_counter.size(); ++difference_index)
    {
        double difference_local = ((double)difference_percent_interval_counter[difference_index] / (double)num_cells) * 100.0;
        file_out << "difference interval " << difference_index << ": " << difference_local << "   (" << difference_percent_interval_counter[difference_index] << "/" << num_cells << ")" << std::endl;
    }

    file_out.close();

    std::cout << "Results written to " << output_file_name << std::endl;
}
