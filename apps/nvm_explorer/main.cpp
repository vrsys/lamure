#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <GL/freeglut.h>
#include "utils.h"
#include "Scene.h"
#include "Controller.h"

#define VERBOSE
#define DEFAULT_PRECISION 15

using namespace utils;

Controller* controller = nullptr;
int time_since_start = 0;
int width_window = 1920;
int height_window = 1080;

void glut_idle();
void glut_display();
void glut_mouse_movement(int x, int y);
void glut_keyboard(unsigned char key, int x, int y);
void glut_keyboard_release(unsigned char key, int x, int y);
void initialize_glut(int argc, char** argv, int width, int height);

char *get_cmd_option(char **begin, char **end, const std::string &option) 
{
    char **it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists (char **begin, char **end, const std::string &option)
{
    return std::find (begin, end, option) != end;
}

int main (int argc, char *argv[])
{
    if (argc == 1 ||
        cmd_option_exists (argv, argv + argc, "-h") ||
        !cmd_option_exists (argv, argv + argc, "-f"))
        {

            std::cout << "Usage: " << argv[0] << "<flags> -f <input_file>.nvm" << std::endl <<
                      "INFO: nvm_explorer " << std::endl <<
                      "\t-f: selects .nvm input file" << std::endl <<
                      "\t    (-f flag is required) " << std::endl <<
                      std::endl;
            return 0;
        }

    std::string name_file_nvm = std::string(get_cmd_option(argv, argv + argc, "-f"));

    std::string ext = name_file_nvm.substr (name_file_nvm.size () - 3);
    if (ext.compare ("nvm") != 0)
        {
            std::cout << "please specify a .nvm file as input" << std::endl;
            return 0;
        }

    initialize_glut(argc, argv, width_window, height_window);

    ifstream in (name_file_nvm);
    vector<Camera> vec_camera;
    vector<Point> vec_point;
    vector<Image> vec_image;

    utils::read_nvm(in, vec_camera, vec_point, vec_image);

    //  std::cout << "cameras: " << vec_camera.size() << std::endl;
    //  std::cout << "points: " << vec_point.size() << std::endl;
    // for(std::vector<Point>::iterator it = vec_point.begin(); it != vec_point.end(); ++it) {
    //     if((*it).get_measurements().size() != 0)
    //         std::cout << (*it).get_measurements().size() << std::endl;
    // }
    //  std::cout << "vec_image: " << vec_image.size() << std::endl;

    Scene scene(vec_camera, vec_point, vec_image);

    controller = new Controller(scene, argv, width_window, height_window);

    glutMainLoop();


    return 0;
}
    
void glut_display()
{
    bool signaled_shutdown = false;
    if (controller != nullptr)
    {

        int new_time_since_start = glutGet(GLUT_ELAPSED_TIME);
        int time_delta = new_time_since_start - time_since_start;
        time_since_start = new_time_since_start;

        signaled_shutdown = controller->update(time_delta);

        glutSwapBuffers();
    }

    if(signaled_shutdown) {
        glutExit();
        exit(0);
    }
}

void initialize_glut(int argc, char** argv, int width, int height)
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

    int wh1 = glutCreateWindow("NVM Explorer");

    glutSetWindow(wh1);

    // glutFullScreenToggle();
    // glutReshCapeFunc(glut_resize);
    glutDisplayFunc(glut_display);
    glutKeyboardFunc(glut_keyboard);
    glutKeyboardUpFunc(glut_keyboard_release);
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
            controller->handle_key_pressed(key);
            break;

    }
}

void glut_keyboard_release(unsigned char key, int x, int y)
{
    controller->handle_key_released(key);
}

void glut_idle()
{
    glutPostRedisplay();
}