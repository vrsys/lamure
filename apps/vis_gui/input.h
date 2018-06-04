//
// Created by moqe3167 on 30/05/18.
//

#ifndef LAMURE_INPUT_H
#define LAMURE_INPUT_H

#include <lamure/ren/camera.h>


struct input {
    float trackball_x_ = 0.f;
    float trackball_y_ = 0.f;
    scm::math::vec2i mouse_;
    scm::math::vec2i prev_mouse_;
    bool brush_mode_ = 0;
    bool brush_clear_ = 0;
    bool gui_lock_ = false;
    lamure::ren::camera::mouse_state mouse_state_;
};

#endif //LAMURE_INPUT_H
