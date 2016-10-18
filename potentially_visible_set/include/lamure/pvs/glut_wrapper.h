#ifndef GLUT_WRAPPER_H
#define GLUT_WRAPPER_H

#include <GL/freeglut.h>

#include "lamure/pvs/management.h"

namespace lamure
{
namespace pvs 
{

class glut_wrapper
{
	public:
		static void initialize(int argc, char** argv, const uint32_t& width, const uint32_t& height, management* manager);
        static void set_management(management* manager);

	private:
		static void resize(int w, int h);
    	static void display();
    	static void keyboard(unsigned char key, int x, int y);
    	static void keyboard_release(unsigned char key, int x, int y);
    	static void mousefunc(int button, int state, int x, int y);
    	static void mousemotion(int x, int y);
    	static void idle();

        static management* manager;
};

}
}

#endif
