#ifndef LAMURE_SCENE_H
#define LAMURE_SCENE_H

#include <vector>

#include "Camera.h"
#include "Point.h"

class Scene {
	private:
	public:
		Scene();
		Scene(std::vector<Camera> camera_vec, std::vector<Point> point_vec, std::vector<Image> images);
};

#endif //LAMURE_SCENE_H
