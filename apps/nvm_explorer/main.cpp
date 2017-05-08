#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <GL/freeglut.h>
#include "utils.h"
#include "Scene.h"
#include "controller/Controller.h"

#define VERBOSE
#define DEFAULT_PRECISION 15

using namespace utils;

Controller* controller = nullptr;

void glut_display();
void initialize_glut(int argc, char** argv, int width, int height);

char *get_cmd_option(char **begin, char **end, const std::string &option) {
    char **it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char **begin, char **end, const std::string &option) {
    return std::find(begin, end, option) != end;
}

int main(int argc, char *argv[]) {
    if (argc == 1 ||
        cmd_option_exists(argv, argv + argc, "-h") ||
        !cmd_option_exists(argv, argv + argc, "-f")) {

        std::cout << "Usage: " << argv[0] << "<flags> -f <input_file>.nvm" << std::endl <<
                  "INFO: nvm_explorer " << std::endl <<
                  "\t-f: selects .nvm input file" << std::endl <<
                  "\t    (-f flag is required) " << std::endl <<
                  std::endl;
        return 0;
    }


    std::string name_file_nvm = std::string(get_cmd_option(argv, argv + argc, "-f"));

    std::string ext = name_file_nvm.substr(name_file_nvm.size() - 3);
    if (ext.compare("nvm") != 0) {
        std::cout << "please specify a .nvm file as input" << std::endl;
        return 0;
    }

    initialize_glut(argc, argv, 800, 600);

    ifstream in(name_file_nvm);
    vector<Camera>        vec_camera;
    vector<Point>        vec_point;
    vector<Image>        vec_image;

    utils::read_nvm(in, vec_camera, vec_point, vec_image);
    std::cout << "cameras: " << vec_camera.size() << std::endl;
    std::cout << "points: " << vec_point.size() << std::endl;
    // for(std::vector<Point>::iterator it = vec_point.begin(); it != vec_point.end(); ++it) {
    //     if((*it).get_measurements().size() != 0)
    //         std::cout << (*it).get_measurements().size() << std::endl;
    // }
    std::cout << "vec_image: " << vec_image.size() << std::endl;

    Scene scene(vec_camera, vec_point, vec_image);

    controller = new Controller(scene, argv);

    glutMainLoop();


    return 0;
}
    

void glut_display()
{
    bool signaled_shutdown = false;
    if (controller != nullptr)
    {
        signaled_shutdown = controller->update();

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

    // glutReshCapeFunc(glut_resize);
    glutDisplayFunc(glut_display);
    // glutKeyboardFunc(glut_keyboard);
    // glutKeyboardUpFunc(glut_keyboard_release);
    // glutMouseFunc(glut_mousefunc);
    // glutMotionFunc(glut_mousemotion);
    // glutIdleFunc(glut_idle);
}


