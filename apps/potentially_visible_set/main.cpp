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
#include <lamure/pvs/grid.h>
#include <lamure/pvs/grid_regular.h>
#include <lamure/pvs/grid_octree.h>
#include <lamure/pvs/grid_optimizer_octree.h>
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

    // Load scene and analyze scene size.
    lamure::pvs::visibility_test* vt = new lamure::pvs::visibility_test_id_histogram_renderer();
    vt->initialize(argc, argv);
    lamure::vec3r scene_dimensions = vt->get_scene_bounds().get_dimensions();

    // Read additional data from input parameters.
    std::string pvs_output_file_path = "";
    std::string grid_type = "";
    unsigned int grid_size = 1;
    unsigned int num_steps = 11;

    namespace po = boost::program_options;
    namespace fs = boost::filesystem;

    const std::string exec_name = (argc > 0) ? fs::basename(argv[0]) : "";
    scm::shared_ptr<scm::core> scm_core(new scm::core(1, argv));

    putenv((char *)"__GL_SYNC_TO_VBLANK=0");

    std::string resource_file_path = "";

    po::options_description desc("Usage: " + exec_name + " [OPTION]... INPUT\n\n"
                               "Allowed Options");
    desc.add_options()
      ("pvs-file,p", po::value<std::string>(&pvs_output_file_path), "specify output file of calculated pvs data")
      ("gridtype", po::value<std::string>(&grid_type)->default_value("octree"), "specify type of grid to store visibility data ('regular' or 'octree')")
      ("gridsize", po::value<unsigned int>(&grid_size)->default_value(1), "specify size/depth of the grid used for the visibility test")
      ("numsteps,n", po::value<unsigned int>(&num_steps)->default_value(11), "specify the number of intervals the occlusion values will be split into");
      ;

    po::variables_map vm;
    auto parsed_options = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
    po::store(parsed_options, vm);
    po::notify(vm);

    if(pvs_output_file_path == "")
    {
        std::cout << "Please specifiy PVS output file path.\n" << desc;
        return 0;
    }

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

    size_t num_cells = grid_size;

    if(grid_type == "regular")
    {
        double cell_size = (std::max(scene_dimensions.x, std::max(scene_dimensions.y, scene_dimensions.z)) / (double)num_cells) * 1.5;
        test_grid = new lamure::pvs::grid_regular(num_cells, cell_size, center, ids);
    }
    else if(grid_type == "octree")
    {
        double cell_size = std::max(scene_dimensions.x, std::max(scene_dimensions.y, scene_dimensions.z)) * 1.5;
        test_grid = new lamure::pvs::grid_octree(num_cells, cell_size, center, ids);
    }
    else
    {
        std::cout << "Unsupported grid type: " << grid_type << std::endl << desc;
        return 0;
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
        optimizer.optimize_grid(test_grid, 0.8f);
    }
    
    std::cout << "Finished grid optimization." << std::endl;

#ifdef PVS_MAIN_MEASURE_PERFORMANCE
    end_time = std::chrono::system_clock::now();
    elapsed_seconds = end_time - start_time;
    double grid_optimization_time = elapsed_seconds.count();
#endif

#ifdef PVS_MAIN_MEASURE_VISIBILITY
    // Write collected visibility data to file.
    std::string visibility_file_path = pvs_output_file_path;
    visibility_file_path.resize(visibility_file_path.size() - 4);
    visibility_file_path += "_visibility.txt";

    lamure::pvs::analyze_grid_visibility(test_grid, num_steps, visibility_file_path);
#endif

#ifdef PVS_MAIN_MEASURE_PERFORMANCE
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
    file_out << "initialization: " << complete_time - (visibility_test_time + grid_optimization_time + save_file_time) << std::endl;
    file_out << "visibility test: " << visibility_test_time << std::endl;
    file_out << "grid optimization: " << grid_optimization_time << std::endl;
    file_out << "save file: " << save_file_time << std::endl;
    file_out << "complete execution: " << complete_time << std::endl;

    file_out.close();
#endif

    vt->shutdown();

    delete test_grid;
    delete vt;

    return 0;
}
