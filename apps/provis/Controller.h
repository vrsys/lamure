#ifndef LAMURE_CONTROLLER_H
#define LAMURE_CONTROLLER_H

#include "Renderer.h"
#include "Scene.h"
#include <GL/glew.h>
#include <chrono>

#include <GLFW/glfw3.h>

#include <vector>

class Controller
{
  private:
    Scene _scene;
    Renderer _renderer;
    scm::shared_ptr<scm::gl::render_device> _device;
    std::map<int, bool> _keys;
    std::map<char, bool> _keys_special;
    double _time_since_start = 0.0;
    bool _is_first_mouse_movement = false; // can be removed
    int time_since_last_brush = 0;
    int time_min_between_brush = 10;
    float _x_last = 0;
    float _y_last = 0;
    float _yaw = 90.0f;
    float _pitch = 0.0f;
    double _time_delta = 0.0;
    bool _mode_navigating = false;
    bool _mode_brushing = false;

    void handle_movements(float time_delta);

  public:
    Controller(Scene const &scene, char **argv, int width_window, int height_window, std::string name_file_lod, std::string name_file_dense,
               lamure::ren::Data_Provenance data_provenance = lamure::ren::Data_Provenance());

    bool update();
    void handle_mouse_movement(int x, int y);
    void handle_mouse_click(int button, int action, int mods, int xpos, int ypos);
    void handle_key_pressed(int key);
    void handle_key_released(int key);
    void handle_key_special_pressed(int key);
    void handle_key_special_released(int key);
};

#endif // LAMURE_CONTROLLER_H
