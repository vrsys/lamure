#include <iostream>

// scism shit
#include <scm/core.h>
#include <scm/log.h>
#include <scm/core/pointer_types.h>
#include <scm/core/io/tools.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>

#include <scm/gl_core.h>

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/manipulators/trackball_manipulator.h>
#include <scm/gl_util/primitives/box.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/primitives/wavefront_obj.h>

// Window library
#include <GL/freeglut.h>

static int winx = 1600;
static int winy = 1024;

int main(int argc, char** argv) {

    // init GLUT and create Window
    glutInit(&argc, argv);
    glutInitContextVersion (4,2);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);

    // set properties from window
    glutCreateWindow("First test with GLUT and SCHISM");
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(winx, winy);

    // register callbacks
    glutDisplayFunc(display);

    // enter GLUT event processing cycle
    glutMainLoop();
    return 0;
}