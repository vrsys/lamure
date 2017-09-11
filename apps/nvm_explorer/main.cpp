#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
// #include <GLFW/glfw3.h>
#include "Controller.h"
#include "Scene.h"
// #include "utils.h"
#include <GL/freeglut.h>
#include <lamure/ren/Data_Provenance.h>
#include <lamure/ren/Item_Provenance.h>

#include <chrono>
#include <lamure/pro/common.h>
#include <lamure/pro/data/DenseCache.h>
#include <lamure/pro/data/DenseStream.h>
#include <lamure/pro/data/SparseCache.h>

#define VERBOSE
#define DEFAULT_PRECISION 15

// using namespace utils;

// KEY-BINDINGS:
// [w,a,s,d,q,e] - move camera through world
// [4,5,6,8] (numpad) - rotate camera
// [j,k] - in-/decrease camera frustra scales
// r - toggle between user-camera and image-camera
// i - toggle one camera/all cameras
// u - switch to previous image-camera
// o - switch to next image-camera
// m - enable/disable image-rendering
// n - toggle between sparse- and dense-rendering
// l - enable/disable line-rendering
// [v,b] - in-/decrease sparse points / servlets radii

// t - toggle lense modes (none, world-lense)
// ctrl + [w,a,s,d,q,e] - move world-lense through world
// ctrl + [x,c] - increase/decrease radius of world-lense

// heatmap
// downsampling images
// ui imgui
// lines

Controller *controller = nullptr;
int time_since_start = 0;
int width_window = 1920;
int height_window = 1080;

void glut_idle();
void glut_display();
void mouse_callback(int button, int state, int x, int y);
void glut_mouse_movement(int x, int y);
void glut_keyboard(unsigned char key, int x, int y);
void glut_keyboard_release(unsigned char key, int x, int y);
void glut_keyboard_special(int key, int x, int y);
void glut_keyboard_special_release(int key, int x, int y);
void initialize_glut(int argc, char **argv, int width, int height);

char *get_cmd_option(char **begin, char **end, const std::string &option)
{
    char **it = std::find(begin, end, option);
    if(it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char **begin, char **end, const std::string &option) { return std::find(begin, end, option) != end; }

int main(int argc, char *argv[])
{
    if(argc == 1 || cmd_option_exists(argv, argv + argc, "-h") || !cmd_option_exists(argv, argv + argc, "-d"))
    {
        std::cout << "Usage: " << argv[0] << " <flags>" << std::endl
                  << "INFO: nvm_explorer " << std::endl
                  << "\t-d: selects .prov input file" << std::endl
                  << "\t    (-d flag is required) " << std::endl
                  << "\t-l: selects .bvh input file" << std::endl
                  << "\t    (-l flag is required) " << std::endl
                  << "\t[-s: selects sparse.prov input file]" << std::endl
                  << "\t    (-s flag is optional) " << std::endl
                  << "\t[-j: selects json input file]" << std::endl
                  << "\t    (-j flag is optional) " << std::endl
                  << std::endl;
        return 0;
    }

    initialize_glut(argc, argv, width_window, height_window);

    Scene scene;
    prov::SparseCache *cache_sparse;
    if(cmd_option_exists(argv, argv + argc, "-s"))
    {
        std::string name_file_sparse = std::string(get_cmd_option(argv, argv + argc, "-s"));
        prov::ifstream in_sparse(name_file_sparse, std::ios::in | std::ios::binary);
        prov::ifstream in_sparse_meta(name_file_sparse + ".meta", std::ios::in | std::ios::binary);
        cache_sparse = new prov::SparseCache(in_sparse, in_sparse_meta);
        cache_sparse->cache();
        in_sparse.close();

        std::vector<prov::Camera> vec_camera = cache_sparse->get_cameras();
        std::vector<prov::SparsePoint> vec_point = cache_sparse->get_points();
        scene = Scene(vec_point, vec_camera);
    }

    std::string name_file_lod = std::string(get_cmd_option(argv, argv + argc, "-l"));
    std::string name_file_dense = std::string(get_cmd_option(argv, argv + argc, "-d"));

    lamure::ren::Data_Provenance data_provenance;
    if(cmd_option_exists(argv, argv + argc, "-j"))
    {
        data_provenance = lamure::ren::Data_Provenance::parse_json(std::string(get_cmd_option(argv, argv + argc, "-j")));
    }

    for(lamure::ren::Item_Provenance const &item : data_provenance.get_items())
    {
        std::cout << item.get_type() << std::endl;
        std::cout << item.get_visualization() << std::endl;
    }
    std::cout << data_provenance.get_size_in_bytes() << std::endl;

    controller = new Controller(scene, argv, width_window, height_window, name_file_lod, name_file_dense, data_provenance);
    std::cout << "start rendering" << std::endl;

    glutMainLoop();

    return 0;
}

void glut_display()
{
    bool signaled_shutdown = false;
    if(controller != nullptr)
    {
        int new_time_since_start = glutGet(GLUT_ELAPSED_TIME);
        int time_delta = new_time_since_start - time_since_start;
        time_since_start = new_time_since_start;

        signaled_shutdown = controller->update(time_delta);

        glutSwapBuffers();
    }

    if(signaled_shutdown)
    {
        glutExit();
        exit(0);
    }
}

void initialize_glut(int argc, char **argv, int width, int height)
{
    glutInit(&argc, argv);
    glutInitContextVersion(3, 0);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA | GLUT_ALPHA | GLUT_MULTISAMPLE);

    glutInitWindowPosition(400, 300);
    glutInitWindowSize(width, height);

    int wh1 = glutCreateWindow("NVM Explorer");

    glutSetWindow(wh1);

    // glutFullScreenToggle();
    // glutReshCapeFunc(glut_resize);
    glutDisplayFunc(glut_display);
    glutKeyboardFunc(glut_keyboard);
    glutKeyboardUpFunc(glut_keyboard_release);
    glutSpecialFunc(glut_keyboard_special);
    glutSpecialUpFunc(glut_keyboard_special_release);
    glutMouseFunc(mouse_callback);
    glutMotionFunc(glut_mouse_movement);
    // glutPassiveMotionFunc(glut_mouse_movement);
    glutIdleFunc(glut_idle);
    // glutFullScreen();
    // glutSetCursor(GLUT_CURSOR_NONE);
}

void glut_mouse_movement(int x, int y) { controller->handle_mouse_movement(x, y); }
void mouse_callback(int button, int state, int x, int y) { controller->handle_mouse_click(button, state, x, y); }

void glut_keyboard_special(int key, int x, int y)
{
    // std::cout << key << std::endl;
    controller->handle_key_special_pressed(key);
}

void glut_keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 27: // escape
        glutExit();
        exit(0);
        break;
    case '.':
        glutFullScreenToggle();
        break;
    default:
        //         std::cout << key << std::endl;
        controller->handle_key_pressed(key);
        break;
    }
}
void glut_keyboard_special_release(int key, int x, int y) { controller->handle_key_special_released(key); }
void glut_keyboard_release(unsigned char key, int x, int y) { controller->handle_key_released(key); }

void glut_idle() { glutPostRedisplay(); }