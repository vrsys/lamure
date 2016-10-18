// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <memory>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

#include <scm/core/math.h>
#include <boost/program_options.hpp>

#include <lamure/ren/camera.h>
#include <lamure/pvs/utils.h>
#include <lamure/pvs/management.h>

#include <GL/freeglut.h>
#include <lamure/pvs/glut_wrapper.h>

#include <lamure/types.h>
#include <lamure/ren/config.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/dataset.h>
#include <lamure/ren/policy.h>

int main(int argc, char** argv)
{
    namespace po = boost::program_options;
    namespace fs = boost::filesystem;

    const std::string exec_name = (argc > 0) ? fs::basename(argv[0]) : "";
    scm::shared_ptr<scm::core> scm_core(new scm::core(1, argv));

    putenv((char *)"__GL_SYNC_TO_VBLANK=0");

    int window_width;
    int window_height;
    unsigned int main_memory_budget;
    unsigned int video_memory_budget ;
    unsigned int max_upload_budget;

    std::string resource_file_path = "";
    std::string measurement_file_path = "";

    po::options_description desc("Usage: " + exec_name + " [OPTION]... INPUT\n\n"
                               "Allowed Options");
    desc.add_options()
      ("help", "print help message")
      ("width,w", po::value<int>(&window_width)->default_value(1920), "specify window width (default=1920)")
      ("height,h", po::value<int>(&window_height)->default_value(1080), "specify window height (default=1080)")
      ("resource-file,f", po::value<std::string>(&resource_file_path), "specify resource input-file")
      ("vram,v", po::value<unsigned>(&video_memory_budget)->default_value(2048), "specify graphics memory budget in MB (default=2048)")
      ("mem,m", po::value<unsigned>(&main_memory_budget)->default_value(4096), "specify main memory budget in MB (default=4096)")
      ("upload,u", po::value<unsigned>(&max_upload_budget)->default_value(64), "specify maximum video memory upload budget per frame in MB (default=64)");
      ;

    po::variables_map vm;

    try
    {    
      auto parsed_options = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
      po::store(parsed_options, vm);
      po::notify(vm);

      std::vector<std::string> to_pass_further = po::collect_unrecognized(parsed_options.options, po::include_positional);
      bool no_input = !vm.count("input") && to_pass_further.empty();

      if (resource_file_path == "") {
        if (vm.count("help") || no_input)
        {
          std::cout << desc;
          return 0;
        }
      }

      // no explicit input -> use unknown options
      if (!vm.count("input") && resource_file_path == "") 
      {
        resource_file_path = "auto_generated.rsc";
        std::fstream ofstr(resource_file_path, std::ios::out);
        if (ofstr.good()) 
        {
          for (auto argument : to_pass_further)
          {
            ofstr << argument << std::endl;
          }
        } else {
          throw std::runtime_error("Cannot open file");
        }
        ofstr.close();
      }
    }
    catch (std::exception& e)
    {
      std::cout << "Warning: No input file specified. \n" << desc;
      return 0;
    }

    // set min and max
    window_width        = std::max(std::min(window_width, 4096), 1);
    window_height       = std::max(std::min(window_height, 2160), 1);
    main_memory_budget  = std::max(int(main_memory_budget), 1);
    video_memory_budget = std::max(int(video_memory_budget), 1);
    max_upload_budget   = std::max(int(max_upload_budget), 64);

    lamure::pvs::glut_wrapper::initialize(argc, argv, window_width, window_height, nullptr);

    std::pair< std::vector<std::string>, std::vector<scm::math::mat4f> > model_attributes;
    std::set<lamure::model_t> visible_set;
    std::set<lamure::model_t> invisible_set;
    model_attributes = read_model_string(resource_file_path, &visible_set, &invisible_set);

    std::vector<scm::math::mat4f> & model_transformations = model_attributes.second;
    std::vector<std::string> const& model_filenames = model_attributes.first;

    lamure::ren::policy* policy = lamure::ren::policy::get_instance();
    policy->set_max_upload_budget_in_mb(max_upload_budget); //8
    policy->set_render_budget_in_mb(video_memory_budget); //2048
    policy->set_out_of_core_budget_in_mb(main_memory_budget); //4096, 8192
    policy->set_window_width(window_width);
    policy->set_window_height(window_height);

    lamure::ren::model_database::get_instance();

    management* management_ = new management(model_filenames, model_transformations, visible_set, invisible_set);
    lamure::pvs::glut_wrapper::set_management(management_);

    //lamure::ren::cut_database::get_instance()->set_cut_update_running(false);

    glutMainLoop();

    if (management_ != nullptr)
    {
        delete lamure::ren::cut_database::get_instance();
        delete lamure::ren::controller::get_instance();
        delete lamure::ren::model_database::get_instance();
        delete lamure::ren::policy::get_instance();
        delete lamure::ren::ooc_cache::get_instance();
    }

    return 0;
}
