// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <algorithm>
#include <string>
#include <vector>

#include <lamure/pvs/visibility_test.h>
#include <lamure/pvs/visibility_test_id_histogram_renderer.h>
#include <lamure/pvs/grid.h>
#include <lamure/pvs/grid_regular.h>
#include <lamure/pvs/pvs_database.h>

#include <lamure/ren/model_database.h>

#include <boost/program_options.hpp>

int main(int argc, char** argv)
{
    // Load scene and analyze scene size.
    lamure::pvs::visibility_test* vt = new lamure::pvs::visibility_test_id_histogram_renderer();
    vt->initialize(argc, argv);
    lamure::vec3r scene_dimensions = vt->get_scene_bounds().get_dimensions();

    // Read additional data from input parameters.
    std::string pvs_output_file_path = "";
    unsigned int grid_size = 1;

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
      ("gridsize,g", po::value<unsigned int>(&grid_size)->default_value(1), "specify size/depth of the grid used for the visibility test");
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

    // Create grid based on scene size.
    size_t num_cells = grid_size;
    double cell_size = (std::max(scene_dimensions.x, std::max(scene_dimensions.y, scene_dimensions.z)) / (double)num_cells) * 1.5;
    
    lamure::vec3r center_bounds = vt->get_scene_bounds().get_center();
    scm::math::vec3d center(center_bounds.x, center_bounds.y, center_bounds.z);
    
    lamure::pvs::grid* test_grid = new lamure::pvs::grid_regular(num_cells, cell_size, center);

    // Debug: allows to test pvs result within pvs view.
    lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
    std::vector<lamure::node_t> ids;
    ids.resize(database->num_models());
    for(lamure::model_t model_index = 0; model_index < ids.size(); ++model_index)
    {
        ids.at(model_index) = database->get_model(model_index)->get_bvh()->get_num_nodes();
    }

    //lamure::pvs::pvs_database* pvs_db = lamure::pvs::pvs_database::get_instance();
    //pvs_db->load_pvs_from_file("/home/tiwo9285/test_bridge.grid", "/home/tiwo9285/test_bridge.pvs", ids);

    // Run visibility test on given scene and grid.
    vt->test_visibility(test_grid);

    // Save grid containing visibility information to file.
    std::string pvs_grid_output_file_path = pvs_output_file_path;
    if(pvs_grid_output_file_path != "")
    {
        pvs_grid_output_file_path.resize(pvs_grid_output_file_path.length() - 3);
        pvs_grid_output_file_path = pvs_grid_output_file_path + "grid";
    }

    std::cout << "Start writing grid file..." << std::endl;
    test_grid->save_grid_to_file(pvs_grid_output_file_path);
    std::cout << "Finished writing grid file.\nStart writing pvs file..." << std::endl;
    test_grid->save_visibility_to_file(pvs_output_file_path, ids);
    std::cout << "Finished writing pvs file." << std::endl;

    vt->shutdown();

    delete test_grid;
    delete vt;

    return 0;
}
