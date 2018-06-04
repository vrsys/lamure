//
// Created by moqe3167 on 30/05/18.
//

#ifndef LAMURE_WINDOW_H
#define LAMURE_WINDOW_H

#include <GLFW/glfw3.h>

struct Window {
    Window() {
        _mouse_button_state = MouseButtonState::IDLE;
    }

    unsigned int _width;
    unsigned int _height;

    GLFWwindow *_glfw_window;

    enum MouseButtonState {
        LEFT = 0,
        WHEEL = 1,
        RIGHT = 2,
        IDLE = 3
    };

    MouseButtonState _mouse_button_state;
};

#endif //LAMURE_WINDOW_H
