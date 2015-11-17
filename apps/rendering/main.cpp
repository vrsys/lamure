// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "utils.h"
#include <lamure/ren/camera.h>

#include <memory>
#include <cmath>
#include <iostream>
#include <string>
#include <algorithm>

#include "management.h"

#include <GL/freeglut.h>

#include <lamure/types.h>
#include <lamure/ren/config.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/lod_point_cloud.h>
#include <lamure/ren/policy.h>

#include <scm/core/math.h>

#include <boost/program_options.hpp>


void InitializeGlut();

void glut_display();
void glut_resize(int w, int h);
void glut_mousefunc(int button, int state, int x, int y);
void glut_mousemotion(int x, int y);
void glut_idle();
void glut_keyboard(unsigned char key, int x, int y);
void glut_keyboard_release(unsigned char key, int x, int y);
void glut_timer(int value);
void glut_close();


void InitializeGlut(int argc, char** argv, uint32_t width, uint32_t height)
{
    glutInit(&argc, argv);
    glutInitContextVersion(4, 4);
    glutInitContextProfile(GLUT_CORE_PROFILE);

	glutSetOption(
        GLUT_ACTION_ON_WINDOW_CLOSE,
		GLUT_ACTION_GLUTMAINLOOP_RETURNS
		);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA | GLUT_ALPHA | GLUT_MULTISAMPLE);

    glutInitWindowPosition(400,300);
    glutInitWindowSize(width, height);

    int wh1 = glutCreateWindow("Point Renderer");

    glutSetWindow(wh1);

    glutReshapeFunc(glut_resize);
    glutDisplayFunc(glut_display);
    glutKeyboardFunc(glut_keyboard);
    glutKeyboardUpFunc(glut_keyboard_release);
    glutMouseFunc(glut_mousefunc);
    glutMotionFunc(glut_mousemotion);
    glutIdleFunc(glut_idle);
}

Management* management_ = nullptr;


char* get_cmd_option(char** begin, char** end, const std::string & option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}


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
    float error_threshold;
    std::string resource_file = "auto_generated.rsc";

    po::options_description desc("Usage: " + exec_name + " [OPTION]... INPUT\n\n"
                               "Allowed Options");
    desc.add_options()
      ("help", "print help message")
      ("width,w", po::value<int>(&window_width)->default_value(1920), "specify window width (default=1920)")
      ("height,h", po::value<int>(&window_height)->default_value(1080), "specify window height (default=1080)")
      ("input,f", po::value<std::string>(&resource_file), "specify input file")
      ("vram,v", po::value<unsigned>(&video_memory_budget)->default_value(2048), "specify graphics memory budget in MB (default=2048)")
      ("mem,m", po::value<unsigned>(&main_memory_budget)->default_value(4096), "specify main memory budget in MB (default=4096)")
      ("upload,u", po::value<unsigned>(&max_upload_budget)->default_value(258), "specify maximum video memory upload budget per frame in MB (default=258)")
      ;

    po::positional_options_description p;
    po::variables_map vm;

    try {    
      auto parsed_options = po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
      po::store(parsed_options, vm);
      po::notify(vm);

      std::vector<std::string> to_pass_further = po::collect_unrecognized(parsed_options.options, po::include_positional);
      bool no_input = !vm.count("input") && to_pass_further.empty();

      if (vm.count("help") || no_input)
      {
        std::cout << desc;
        return 0;
      }

      // no explicit input -> use unknown options
      if (!vm.count("input")) 
      {
        std::fstream ofstr(resource_file, std::ios::out);
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
    } catch (std::exception& e) {
      std::cout << "Warning: No input file specified. \n" << desc;
      return 0;
    }

    // set min and max
    window_width        = std::max(std::min(window_width, 1920), 800);
    window_height       = std::max(std::min(window_width, 1080), 600);
    main_memory_budget  = std::max(int(main_memory_budget), 1024);
    video_memory_budget = std::max(int(video_memory_budget), 512);
    max_upload_budget   = std::max(int(max_upload_budget), 64);

    InitializeGlut(argc, argv, window_width, window_height);

    std::pair< std::vector<std::string>, std::vector<scm::math::mat4f> > model_attributes;
    std::set<lamure::model_t> visible_set;
    std::set<lamure::model_t> invisible_set;
    model_attributes = ReadModelString(resource_file, &visible_set, &invisible_set);

    //std::string scene_name;
    //CreateSceneNameFromVector(model_attributes.first, scene_name);
    std::vector<scm::math::mat4f> & model_transformations = model_attributes.second;
    std::vector<std::string> const& model_filenames = model_attributes.first;

    lamure::ren::Policy* policy = lamure::ren::Policy::GetInstance();
    policy->set_max_upload_budget_in_mb(max_upload_budget); //8
    policy->set_render_budget_in_mb(video_memory_budget); //2048
    policy->set_out_of_core_budget_in_mb(main_memory_budget); //4096, 8192

    lamure::ren::ModelDatabase* database = lamure::ren::ModelDatabase::GetInstance();
    database->set_window_width(window_width);
    database->set_window_height(window_height);
    management_ = new Management(model_filenames, model_transformations, visible_set, invisible_set);

    glutMainLoop();


    if (management_ != nullptr)
    {
#ifdef LAMURE_ENABLE_INFO
        std::cout << "memory cleanup...(1)" << std::endl;
#endif
        delete lamure::ren::CutDatabase::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted cut database" << std::endl;
#endif
        delete lamure::ren::Controller::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted controller" << std::endl;
#endif
        delete lamure::ren::ModelDatabase::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted model database" << std::endl;
#endif
        delete lamure::ren::Policy::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted policy" << std::endl;
#endif
        delete lamure::ren::OocCache::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted ooc cache" << std::endl;
#endif
    }



    return 0;
}



void glut_display()
{

    if (management_ != nullptr)
    {
        management_->MainLoop();
        glutSwapBuffers();
    }

}


void glut_resize(int w, int h)
{

    if (management_ != nullptr)
    {
        management_->DispatchResize(w, h);
    }

}

void glut_mousefunc(int button, int state, int x, int y)
{
    if (management_ != nullptr)
    {
        management_->RegisterMousePresses(button, state, x, y);
    }


}

void glut_mousemotion(int x, int y)
{
    if (management_ != nullptr)
    {
        management_->UpdateTrackball(x, y);
    }
}

void glut_idle()
{
    glutPostRedisplay();
}

void Cleanup()
{

    if (management_ != nullptr)
    {
        delete management_;
        management_ = nullptr;

#ifdef LAMURE_ENABLE_INFO
        std::cout << "memory cleanup...(1)" << std::endl;
#endif
        delete lamure::ren::CutDatabase::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted cut database" << std::endl;
#endif
        delete lamure::ren::Controller::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted controller" << std::endl;
#endif
        delete lamure::ren::ModelDatabase::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted model database" << std::endl;
#endif
        delete lamure::ren::Policy::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted policy" << std::endl;
#endif
        delete lamure::ren::OocCache::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted ooc cache" << std::endl;
#endif
    }

}

void glut_close()
{

    if (management_ != nullptr)
    {
        delete management_;
        management_ = nullptr;

#ifdef LAMURE_ENABLE_INFO
        std::cout << "memory cleanup...(1)" << std::endl;
#endif
        delete lamure::ren::CutDatabase::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted cut database" << std::endl;
#endif
        delete lamure::ren::Controller::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted controller" << std::endl;
#endif
        delete lamure::ren::ModelDatabase::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted model database" << std::endl;
#endif
        delete lamure::ren::Policy::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted policy" << std::endl;
#endif
        delete lamure::ren::OocCache::GetInstance();
#ifdef LAMURE_ENABLE_INFO
        std::cout << "deleted ooc cache" << std::endl;
#endif
    }
}


void glut_keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
        case 27:
            //Cleanup();
            glutExit();
            exit(0);
            break;
        case '.':
            glutFullScreenToggle();
            break;

        default:
            if (management_ != nullptr)
            {
                management_->DispatchKeyboardInput(key);
            }
            break;


    }
}

void glut_keyboard_release(unsigned char key, int x, int y)
{

}


