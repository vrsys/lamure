#ifndef LAMURE_CONTROLLER_H
#define LAMURE_CONTROLLER_H

#include <vector>
#include "../Renderer.h"
#include "../Scene.h"	

class Controller {
	private:
		Scene _scene;
		Renderer _renderer;
	public:
		Controller(Scene scene, char** argv);
		
		bool update();

};

#endif //LAMURE_CONTROLLER_H
