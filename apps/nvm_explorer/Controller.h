#ifndef LAMURE_CONTROLLER_H
#define LAMURE_CONTROLLER_H

#include "Renderer.h"
#include "Scene.h"
#include <vector>

class Controller
{
  private:
    Scene _scene;
    Renderer _renderer;
    scm::shared_ptr<scm::gl::render_device> _device;
    bool _keys[1024];
    bool _keys_special[1024];
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    bool _is_first_mouse_movement = false;
    float _x_last = 0;
    float _y_last = 0;
    float _yaw = 90.0f;
    float _pitch = 0.0f;
    bool tmp = true;

    void handle_movements(int time_delta);

  public:
    Controller(Scene const &scene, char **argv, int width_window, int height_window);

    bool update(int time_delta);
    void handle_mouse_movement(int x, int y);
    void handle_key_pressed(char key);
    void handle_key_special_pressed(int key);
    void handle_key_special_released(int key);
    void handle_key_released(char key);
};

#endif // LAMURE_CONTROLLER_H
