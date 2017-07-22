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
#include <lamure/pro/data/LoDMetaStream.h>
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

bool write_dummy_binary_file()
{
    std::cout << "sizes: " << std::endl;
    std::cout << "size of int: " << sizeof(int) << std::endl;
    std::cout << "size of float: " << sizeof(float) << std::endl;
    std::cout << "size of double: " << sizeof(double) << std::endl;
    std::ofstream ofile("dummy_binary_file.bin", std::ios::binary);
    for(int i = 0; i < 36022860 / 4.0f; ++i)
    // for (int i = 0; i < 4434885; ++i)
    {
        float f = 0.0f;
        ofile.write((char *)&f, sizeof(float));
        // ofile.write((char*) &f, sizeof(float));
        // f = 1.0f;
        // ofile.write((char*) &f, sizeof(float));

        // const float f = double(rand()) / double(RAND_MAX);
        // ofile.write((char*) &f, sizeof(float));

        // const float f = double(rand()) / double(RAND_MAX);
        // ofile.write((char*) &f, sizeof(float));

        // const double f = double(rand()) / double(RAND_MAX);
        // ofile.write((char*) &f, sizeof(double));
    }
    return true;
}

bool cmd_option_exists(char **begin, char **end, const std::string &option) { return std::find(begin, end, option) != end; }

int main(int argc, char *argv[])
{
    lamure::ren::Data_Provenance data_provenance;

    lamure::ren::Item_Provenance item_float(
        // lamure::ren::Item_Provenance::type_item::TYPE_VEC3F,
        lamure::ren::Item_Provenance::type_item::TYPE_FLOAT, lamure::ren::Item_Provenance::visualization_item::VISUALIZATION_COLOR);

    data_provenance.add_item(item_float);
    data_provenance.add_item(item_float);
    data_provenance.add_item(item_float);

    data_provenance.add_item(item_float);
    data_provenance.add_item(item_float);
    data_provenance.add_item(item_float);
    // lamure::ren::Item_Provenance item_float(
    //     lamure::ren::Item_Provenance::type_item::TYPE_FLOAT,
    //     lamure::ren::Item_Provenance::visualization_item::VISUALIZATION_COLOR
    // );
    // data_provenance.add_item(item_float);

    // lamure::ren::Item_Provenance item_vec3f(
    //     lamure::ren::Item_Provenance::type_item::TYPE_VEC3F,
    //     lamure::ren::Item_Provenance::visualization_item::VISUALIZATION_COLOR
    // );
    // data_provenance.add_item(item_vec3f);

    // lamure::ren::Item_Provenance item_int(
    //     lamure::ren::Item_Provenance::type_item::TYPE_INT,
    //     lamure::ren::Item_Provenance::visualization_item::VISUALIZATION_COLOR
    // );
    // data_provenance.add_item(item_int);

    // std::cout << "size: " << data_provenance.get_size_in_bytes() << std::endl;

    // return 1;
    // return write_dummy_binary_file();

    if(argc == 1 || cmd_option_exists(argv, argv + argc, "-h") || !cmd_option_exists(argv, argv + argc, "-d"))
    {
        std::cout << "Usage: " << argv[0] << "<flags> -s <input_file>.nvm" << std::endl
                  << "INFO: nvm_explorer " << std::endl
                  << "\t-d: selects .bvh input file" << std::endl
                  << "\t    (-d flag is required) " << std::endl
                  << "\t[-s: selects sparse.prov input file]" << std::endl
                  << "\t    (-s flag is optional) " << std::endl
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

    // std::string name_file_nvm = std::string(get_cmd_option(argv, argv + argc, "-s"));

    // std::string ext = name_file_nvm.substr(name_file_nvm.size() - 3);
    // if(ext.compare("nvm") != 0)
    // {
    //     std::cout << "please specify a .nvm file as input" << std::endl;
    //     return 0;
    // }

    // if(!glfwInit())
    // {
    //     // Initialization failed
    // }

    // ifstream in(name_file_nvm);
    // vector<Point> vec_point_sparse;
    // vector<Point> vec_point_dense;
    // vector<Image> vec_image;

    // utils::read_nvm(in, vec_camera, vec_point_sparse, vec_image);

    // ifstream in_ply(name_file_nvm + ".cmvs/00/models/option-0000.ply", ifstream::binary);

    // utils::read_ply(in_ply, vec_point_dense);

    //  std::cout << "cameras: " << vec_camera.size() << std::endl;
    //  std::cout << "points: " << vec_point.size() << std::endl;
    // for(std::vector<Point>::iterator it = vec_point.begin(); it != vec_point.end(); ++it) {
    //     if((*it).get_measurements().size() != 0)
    //         std::cout << (*it).get_measurements().size() << std::endl;
    // }
    //  std::cout << "vec_image: " << vec_image.size() << std::endl;

    // Scene scene = Scene(in_sparse,in_sparse_meta);

    // Scene scene(cache_sparse);

    controller = new Controller(scene, argv, width_window, height_window, std::string(get_cmd_option(argv, argv + argc, "-d")), data_provenance);
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
    // glutMouseFunc(mouse_callback);
    glutPassiveMotionFunc(glut_mouse_movement);
    glutIdleFunc(glut_idle);
    // glutFullScreen();
    // glutSetCursor(GLUT_CURSOR_NONE);
}

void glut_mouse_movement(int x, int y)
{
    // std::cout << x << std::endl;
    // controller->handle_mouse_movement(x, y);
    // glutWarpPointer(1920/2, 1080/2);
}
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