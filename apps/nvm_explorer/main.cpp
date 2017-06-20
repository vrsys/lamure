#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
// #include <GLFW/glfw3.h>
#include "Controller.h"
#include "Scene.h"
#include "utils.h"
#include <GL/freeglut.h> 
#include <lamure/ren/Data_Provenance.h>
#include <lamure/ren/Item_Provenance.h>


#define VERBOSE
#define DEFAULT_PRECISION 15

using namespace utils;

// KEY-BINDINGS:
// [w,a,s,d,q,e] - move camera through world
// [4,5,6,8] (numpad) - rotate camera
// [j,k] - in-/decrease camera frustra scales
// i - toggle between user-camera and image-camera
// u - switch to previous image-camera
// o - switch to next image-camera
// m - enable/disable image-rendering
// n - toggle between sparse- and dense-rendering
// [v,b] - in-/decrease servlets radii

// t - toggle lense modes (none, world-lense, screen-lense)
// alt + [w,a,s,d] - move screen-lense over screen
// shift + [w,a,s,d,q,e] - move world-lense through world
// shift + [x,c] - increase/decrease radius of world-lense
// alt + [x,c] - in-/decrease radius of screen-lense

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
    for (int i = 0; i < 4434885; ++i)
    {
        float f = 0.0f;
        ofile.write((char*) &f, sizeof(float));
        ofile.write((char*) &f, sizeof(float));
        f = 1.0f;
        ofile.write((char*) &f, sizeof(float));

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
        lamure::ren::Item_Provenance::type_item::TYPE_FLOAT,
        lamure::ren::Item_Provenance::visualization_item::VISUALIZATION_COLOR
    );
    data_provenance.add_item(item_float);

    lamure::ren::Item_Provenance item_vec3f(
        lamure::ren::Item_Provenance::type_item::TYPE_VEC3F,
        lamure::ren::Item_Provenance::visualization_item::VISUALIZATION_COLOR
    );
    data_provenance.add_item(item_vec3f);

    lamure::ren::Item_Provenance item_int(
        lamure::ren::Item_Provenance::type_item::TYPE_INT,
        lamure::ren::Item_Provenance::visualization_item::VISUALIZATION_COLOR
    );
    data_provenance.add_item(item_int);

    std::cout << "size: " << data_provenance.get_size_in_bytes() << std::endl;
    // return 1;
    // return write_dummy_binary_file();

    if(argc == 1 || cmd_option_exists(argv, argv + argc, "-h") || !cmd_option_exists(argv, argv + argc, "-f"))
    {
        std::cout << "Usage: " << argv[0] << "<flags> -f <input_file>.nvm" << std::endl
                  << "INFO: nvm_explorer " << std::endl
                  << "\t-f: selects .nvm input file" << std::endl
                  << "\t    (-f flag is required) " << std::endl
                  << std::endl;
        return 0;
    }

    std::string name_file_nvm = std::string(get_cmd_option(argv, argv + argc, "-f"));

    std::string ext = name_file_nvm.substr(name_file_nvm.size() - 3);
    if(ext.compare("nvm") != 0)
    {
        std::cout << "please specify a .nvm file as input" << std::endl;
        return 0;
    }

    // if(!glfwInit())
    // {
    //     // Initialization failed
    // }

    initialize_glut(argc, argv, width_window, height_window);

    ifstream in(name_file_nvm);
    vector<Camera> vec_camera;
    vector<Point> vec_point_sparse;
    vector<Point> vec_point_dense;
    vector<Image> vec_image;

    utils::read_nvm(in, vec_camera, vec_point_sparse, vec_image);

    ifstream in_ply(name_file_nvm + ".cmvs/00/models/option-0000.ply", ifstream::binary);

    utils::read_ply(in_ply, vec_point_dense);

    //  std::cout << "cameras: " << vec_camera.size() << std::endl;
    //  std::cout << "points: " << vec_point.size() << std::endl;
    // for(std::vector<Point>::iterator it = vec_point.begin(); it != vec_point.end(); ++it) {
    //     if((*it).get_measurements().size() != 0)
    //         std::cout << (*it).get_measurements().size() << std::endl;
    // }
    //  std::cout << "vec_image: " << vec_image.size() << std::endl;

    Scene scene(vec_camera, vec_point_sparse, vec_image);

    controller = new Controller(scene, argv, width_window, height_window);
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
    case 27:
        glutExit();
        exit(0);
        break;
    case '.':
        glutFullScreenToggle();
    default:
        // std::cout << key << std::endl;
        controller->handle_key_pressed(key);
        break;
    }
}

void glut_keyboard_special_release(int key, int x, int y) { controller->handle_key_special_released(key); }
void glut_keyboard_release(unsigned char key, int x, int y) { controller->handle_key_released(key); }

void glut_idle() { glutPostRedisplay(); }