// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <algorithm>
#include <string>
#include <vector>
#include <chrono>
#include <fstream>

#include <lamure/pvs/visibility_test.h>
#include <lamure/pvs/visibility_test_id_histogram_renderer.h>
#include <lamure/pvs/visibility_test_simple_randomized_id_histogram_renderer.h>

#include <lamure/pvs/grid.h>

#include <lamure/pvs/grid_optimizer_octree.h>
#include <lamure/pvs/grid_optimizer_octree_hierarchical.h>

#include <lamure/pvs/pvs_database.h>
#include "lamure/pvs/pvs_utils.h"

#include <lamure/ren/model_database.h>

#include <boost/program_options.hpp>

#define PVS_MAIN_MEASURE_PERFORMANCE
#define PVS_MAIN_MEASURE_VISIBILITY           // Will output some info on the calculated visibility into a special file.

int main(int argc, char** argv)
{
#ifdef PVS_MAIN_MEASURE_PERFORMANCE
    std::chrono::time_point<std::chrono::system_clock> start_time_complete, end_time_complete;
    start_time_complete = std::chrono::system_clock::now();
#endif

    // Read additional data from input parameters.
    std::string pvs_output_file_path = "";
    std::string visibility_test_type = "";
    std::string grid_type = "";
    unsigned int grid_size = 1;
    unsigned int num_steps = 11;
    double oversize_factor = 1.5;
    float optimization_threshold = 1.0f;

    namespace po = boost::program_options;
    namespace fs = boost::filesystem;

    const std::string exec_name = (argc > 0) ? fs::basename(argv[0]) : "";
    //scm::shared_ptr<scm::core> scm_core(new scm::core(1, argv));

    putenv((char *)"__GL_SYNC_TO_VBLANK=0");

    std::string resource_file_path = "";

    po::options_description desc("Usage: " + exec_name + " [OPTION]... INPUT\n\n"
                               "Allowed Options");
    desc.add_options()
      ("pvs-file,p", po::value<std::string>(&pvs_output_file_path), "specify output file of calculated pvs data")
      ("vistest", po::value<std::string>(&visibility_test_type)->default_value("hr"), "specify type of visibility test to be used. (histogram renderer 'hr', simple randomized histogram renderer 'srhr')")
      ("gridtype", po::value<std::string>(&grid_type)->default_value("octree"), "specify type of grid to store visibility data ('regular', 'regular_compressed', 'octree', 'octree_hierarchical', 'octree_hierarchical_v2', 'octree_hierarchical_v3')")
      ("gridsize", po::value<unsigned int>(&grid_size)->default_value(1), "specify size/depth of the grid used for the visibility test (depends on chosen grid type)")
      ("oversize", po::value<double>(&oversize_factor)->default_value(1.5), "factor the grid bounds will be scaled by, default is 1.5 (grid bounds will exceed scene bounds by factor of 1.5)")
      ("optithresh", po::value<float>(&optimization_threshold)->default_value(1.0f), "specify the threshold at which common data are converged. Default is 1.0, which means data must be 100 percent equal.")
      ("numsteps,n", po::value<unsigned int>(&num_steps)->default_value(11), "specify the number of intervals the occlusion values will be split into (visibility analysis only)");
      ;

    // Parse additonal passed parameters.
    po::variables_map vm;
    auto parsed_options = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
    po::store(parsed_options, vm);
    po::notify(vm);

    // Check for valid file path to save pvs-file to.
    if(pvs_output_file_path == "")
    {
        std::cout << "Please specifiy PVS output file path.\n" << desc;
        return 0;
    }

    // Load scene and analyze scene size by choosing and analyzing the visibility test.
    lamure::pvs::visibility_test* vt = nullptr;
    
    if(visibility_test_type == "hr")
    {
        vt = new lamure::pvs::visibility_test_id_histogram_renderer();
    }
    else if(visibility_test_type == "srhr")
    {
        vt = new lamure::pvs::visibility_test_simple_randomized_id_histogram_renderer();
    }
    else
    {
        std::cout << "Invalid visibility test: " << visibility_test_type << ".\n" << desc;
        return 0;
    }

    vt->initialize(argc, argv);
    lamure::vec3r scene_dimensions = vt->get_scene_bounds().get_dimensions();

    lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
    
    // Models may have their own translation, so calculate their center and add it to the scene bounds calculated from the bounding boxes.
    lamure::vec3r translation_center(0.0, 0.0, 0.0);
    for(lamure::model_t model_index = 0; model_index < database->num_models(); ++model_index)
    {
        translation_center += database->get_model(model_index)->get_bvh()->get_translation();
    }
    translation_center /= database->num_models();

    lamure::vec3r center_bounds = vt->get_scene_bounds().get_center();
    scm::math::vec3d center(center_bounds);

    // Knowledge about how many models containing how many nodes is required to save pvs data.
    std::vector<lamure::node_t> ids;
    ids.resize(database->num_models());
    for(lamure::model_t model_index = 0; model_index < ids.size(); ++model_index)
    {
        ids.at(model_index) = database->get_model(model_index)->get_bvh()->get_num_nodes();
    }

    // Create grid based on scene size.
    lamure::pvs::grid* test_grid = nullptr;

    // Note how num_cells is interpreted by grid itself (e.g. octrees use it as depth, regular grids as cells per dimension)
    size_t num_cells = grid_size;
    double grid_bounds_size = std::max(scene_dimensions.x, std::max(scene_dimensions.y, scene_dimensions.z)) * oversize_factor;

    // TODO: calculate num cells of each axis for irregular grid here.
    double cell_size = grid_bounds_size / (double)num_cells;
    size_t cells_x = std::ceil((scene_dimensions.x * oversize_factor) / cell_size);
    size_t cells_y = std::ceil((scene_dimensions.y * oversize_factor) / cell_size);
    size_t cells_z = std::ceil((scene_dimensions.z * oversize_factor) / cell_size);

    test_grid = lamure::pvs::pvs_database::get_instance()->create_grid_by_type(grid_type, cells_x, cells_y, cells_z, grid_bounds_size, center, ids);

    if(test_grid == nullptr)
    {
        std::cout << "Unsupported grid type: " << grid_type << std::endl << desc;
        return 0;
    }

    // Make sure optimization threshold is within certain bounds (0-100%).
    if(optimization_threshold > 1.0f)
    {
        optimization_threshold = 1.0f;
    }
    else if(optimization_threshold < 0.0f)
    {
        optimization_threshold = 0.0f;
    }

    /*std::string pvs_grid_output_file_path = pvs_output_file_path;
    if(pvs_grid_output_file_path != "")
    {
        pvs_grid_output_file_path.resize(pvs_grid_output_file_path.length() - 3);
        pvs_grid_output_file_path = pvs_grid_output_file_path + "grid";
    }

    lamure::pvs::pvs_database* pvs_db = lamure::pvs::pvs_database::get_instance();
    pvs_db->load_pvs_from_file(pvs_output_file_path, pvs_grid_output_file_path);*/

#ifdef PVS_MAIN_MEASURE_PERFORMANCE
    std::chrono::time_point<std::chrono::system_clock> start_time, end_time;
    start_time = std::chrono::system_clock::now();
#endif

    // Run visibility test on given scene and grid.
    vt->test_visibility(test_grid);

#ifdef PVS_MAIN_MEASURE_PERFORMANCE
    end_time = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_time - start_time;
    double visibility_test_time = elapsed_seconds.count();
#endif

    // Save grid containing visibility information to file.
    std::string pvs_grid_output_file_path = pvs_output_file_path;
    if(pvs_grid_output_file_path != "")
    {
        pvs_grid_output_file_path.resize(pvs_grid_output_file_path.length() - 3);
        pvs_grid_output_file_path = pvs_grid_output_file_path + "grid";
    }

#ifdef PVS_MAIN_MEASURE_PERFORMANCE
    start_time = std::chrono::system_clock::now();
#endif

    // Optimize grid by reducing amount of view cells if possible.
    std::cout << "Start grid optimization..." << std::endl;

    if(grid_type == "octree")
    {
        lamure::pvs::grid_optimizer_octree optimizer;
        optimizer.optimize_grid(test_grid, optimization_threshold);
    }
    else if(grid_type == "octree_hierarchical" || grid_type == "octree_hierarchical_v2")
    {
        lamure::pvs::grid_optimizer_octree_hierarchical optimizer;
        optimizer.optimize_grid(test_grid, optimization_threshold);
    }
    
    std::cout << "Finished grid optimization." << std::endl;

#ifdef PVS_MAIN_MEASURE_PERFORMANCE
    end_time = std::chrono::system_clock::now();
    elapsed_seconds = end_time - start_time;
    double grid_optimization_time = elapsed_seconds.count();
    start_time = std::chrono::system_clock::now();
#endif

#ifdef PVS_MAIN_MEASURE_VISIBILITY
    // Write collected visibility data to file.
    std::string visibility_file_path = pvs_output_file_path;
    visibility_file_path.resize(visibility_file_path.size() - 4);
    visibility_file_path += "_visibility.txt";

    lamure::pvs::analyze_grid_visibility(test_grid, num_steps, visibility_file_path);
#endif

#ifdef PVS_MAIN_MEASURE_PERFORMANCE
    end_time = std::chrono::system_clock::now();
    elapsed_seconds = end_time - start_time;
    double visibility_analysis_time = elapsed_seconds.count();
    start_time = std::chrono::system_clock::now();
#endif

    // Write grid and contained visibility data to files.
    std::cout << "Start writing grid file..." << std::endl;
    test_grid->save_grid_to_file(pvs_grid_output_file_path);
    std::cout << "Finished writing grid file.\nStart writing pvs file..." << std::endl;
    test_grid->save_visibility_to_file(pvs_output_file_path);
    std::cout << "Finished writing pvs file." << std::endl;

#ifdef PVS_MAIN_MEASURE_PERFORMANCE
    end_time = std::chrono::system_clock::now();
    elapsed_seconds = end_time - start_time;
    double save_file_time = elapsed_seconds.count();

    end_time_complete = std::chrono::system_clock::now();
    elapsed_seconds = end_time_complete - start_time_complete;
    double complete_time = elapsed_seconds.count();

    std::string performance_file_path = pvs_output_file_path;
    performance_file_path.resize(performance_file_path.size() - 4);
    performance_file_path += "_performance.txt";

    std::ofstream file_out;
    file_out.open(performance_file_path, std::ios::app);

    file_out << "---------- main performance in seconds ----------" << std::endl;
    file_out << "initialization: " << complete_time - (visibility_test_time + grid_optimization_time + visibility_analysis_time + save_file_time) << std::endl;
    file_out << "visibility test: " << visibility_test_time << std::endl;
    file_out << "grid optimization: " << grid_optimization_time << std::endl;
#ifdef PVS_MAIN_MEASURE_VISIBILITY
    file_out << "visibility analysis: " << visibility_analysis_time << std::endl;
#endif    
    file_out << "save file: " << save_file_time << std::endl;
    file_out << "complete execution: " << complete_time << std::endl;

    file_out.close();
#endif

    vt->shutdown();

    delete test_grid;
    delete vt;

    return 0;
}
