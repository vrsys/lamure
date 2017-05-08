#ifndef LAMURE_SCENE_H
#define LAMURE_SCENE_H

#include <vector>

#include "Camera.h"
#include "Point.h"

class Scene {
	private:
		std::vector<Camera> _vector_camera;
		std::vector<Point> _vector_point;
		std::vector<Image> _vector_image;
		
	public:
		Scene();
		Scene(std::vector<Camera> vector_camera, std::vector<Point> vector_point, std::vector<Image> vector_image);
};

#endif //LAMURE_SCENE_H
