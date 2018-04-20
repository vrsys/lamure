// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "Controller.h"
#include "Scene.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
// #include "utils.h"
#include <lamure/ren/data_provenance.h>
#include <lamure/ren/item_provenance.h>
#include <lamure/ren/policy.h>

#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"
#include <lamure/prov/common.h>
#include <lamure/prov/dense_cache.h>
#include <lamure/prov/dense_stream.h>
#include <lamure/prov/sparse_cache.h>

#define VERBOSE
#define DEFAULT_PRECISION 15

// using namespace utils;

// KEY-BINDINGS:
// [w,a,s,d,q,e] - move camera through world
// [4,5,6,8] (numpad) - rotate camera
// [j,k] - in-/decrease camera frustra scales
// r - toggle between user-camera and image-camera
// i - toggle one camera/all cameras
// u - switch to previous image-camera
// o - switch to next image-camera
// m - enable/disable image-rendering
// n - toggle between sparse- and dense-rendering
// l - enable/disable line-rendering
// [v,b] - in-/decrease sparse points / servlets radii

// t - toggle lense modes (none, world-lense)
// ctrl + [w,a,s,d,q,e] - move world-lense through world
// ctrl + [x,c] - increase/decrease radius of world-lense

// heatmap
// downsampling images
// ui imgui
// lines

Controller *controller = nullptr;
int time_since_start = 0;
int width_window = -1;
int height_window = -1;

// void glut_idle();
// void glut_display();
// void mouse_callback(int button, int state, int x, int y);
// void glut_mouse_movement(int x, int y);
// // void glut_mouse_movement_passive(int x, int y);
// void glut_keyboard(unsigned char key, int x, int y);
// void glut_keyboard_release(unsigned char key, int x, int y);
// void glut_keyboard_special(int key, int x, int y);
// void glut_keyboard_special_release(int key, int x, int y);
// void initialize_glut(int argc, char **argv, int width, int height);

char *get_cmd_option(char **begin, char **end, const std::string &option)
{
    char **it = std::find(begin, end, option);
    if(it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char **begin, char **end, const std::string &option) { return std::find(begin, end, option) != end; }

void processInput(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // std::cout << key << std::endl;
    switch(action)
    {
    case GLFW_PRESS:
        switch(key)
        {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
        case GLFW_KEY_PERIOD:
            break;
        default:
            controller->handle_key_pressed(key);
            break;
        }
        break;
    case GLFW_RELEASE:
        controller->handle_key_released(key);
        break;
    }

    // controller->handle_key_special_pressed(key);
    // controller->handle_key_special_released(key);
}

static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) { controller->handle_mouse_movement((int)xpos, (int)ypos); }
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    controller->handle_mouse_click(button, action, mods, (int)xpos, (int)ypos);
}

GLFWwindow *init_glfw_and_glew()
{
    if(!glfwInit())
    {
        std::cout << "GLFW ERROR";
    }

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    glfwWindowHint(GLFW_DECORATED, GL_FALSE);
    width_window = mode->width;
    height_window = mode->height;

    GLFWwindow *window = glfwCreateWindow(mode->width, mode->height, "ProvVis", monitor, NULL);

    if(window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);
    glewInit();

    // glfwSetKeyCallback(window, processInput);

    return window;
}

Controller *load_scene_depending_on_arguments(int argc, char *argv[])
{


    lamure::ren::policy *policy = lamure::ren::policy::get_instance();
    policy->set_max_upload_budget_in_mb(64);

    policy->set_render_budget_in_mb(1024*4);
    // policy->set_render_budget_in_mb(1024 * 6);
    // policy->set_render_budget_in_mb(1024 * 40);
    // policy->set_render_budget_in_mb(256);

    policy->set_out_of_core_budget_in_mb(1024*8);
    // policy->set_out_of_core_budget_in_mb(1024 * 3);
    // policy->set_out_of_core_budget_in_mb(1024 * 20);
    // policy->set_out_of_core_budget_in_mb(256);

    Scene scene;
    if(cmd_option_exists(argv, argv + argc, "-sparse"))
    {
        std::string name_file_sparse = std::string(get_cmd_option(argv, argv + argc, "-sparse"));
        std::ifstream in_sparse(name_file_sparse, std::ios::in | std::ios::binary);
        std::ifstream in_sparse_meta(name_file_sparse + ".meta", std::ios::in | std::ios::binary);
        lamure::prov::SparseCache cache_sparse = lamure::prov::SparseCache(in_sparse, in_sparse_meta);
        cache_sparse.cache();
        in_sparse.close();

        std::vector<lamure::prov::Camera> vec_camera = cache_sparse.get_cameras();
        std::vector<lamure::prov::SparsePoint> vec_point = cache_sparse.get_points();
        scene = Scene(vec_point, vec_camera);
    }

    std::string name_file_lod = std::string(get_cmd_option(argv, argv + argc, "-lod"));
    std::string name_file_tree = std::string(get_cmd_option(argv, argv + argc, "-tree"));
    std::string image_directory = std::string(get_cmd_option(argv, argv + argc, "-fotos"));

    lamure::ren::Data_Provenance data_provenance;
    if(cmd_option_exists(argv, argv + argc, "-json"))
    {
        data_provenance = lamure::ren::Data_Provenance::parse_json(std::string(get_cmd_option(argv, argv + argc, "-json")));
    }

    for(lamure::ren::Item_Provenance const &item : data_provenance.get_items())
    {
        std::cout << item.get_type() << std::endl;
        std::cout << item.get_visualization() << std::endl;
    }
    std::cout << "size of provenance: " << data_provenance.get_size_in_bytes() << std::endl;

    controller = new Controller(scene, argv, width_window, height_window, name_file_lod, name_file_tree, image_directory, data_provenance);
    return controller;
}

void set_styles();
int main(int argc, char *argv[])
{
    if(argc == 1 || cmd_option_exists(argv, argv + argc, "-h")
     || !cmd_option_exists(argv, argv + argc, "-lod")
     || !cmd_option_exists(argv, argv + argc, "-dense")
     || !cmd_option_exists(argv, argv + argc, "-sparse")
     || !cmd_option_exists(argv, argv + argc, "-json")
     || !cmd_option_exists(argv, argv + argc, "-tree")
     || !cmd_option_exists(argv, argv + argc, "-fotos"))
    {
        std::cout << "Usage: " << argv[0] << " <flags>" << std::endl
                  << "INFO: nvm_explorer " << std::endl
                  << "\t-lod: select .bvh file" << std::endl
                  << "\t-sparse: select sparse.prov file" << std::endl
                  << "\t-json: select provenance_data_structure.json file" << std::endl
                  << "\t-tree: select tree.prov file" << std::endl
                  << "\t-fotos: select foto directory" << std::endl
                  << std::endl;
        return 0;
    }

    
    GLFWwindow *window = init_glfw_and_glew();

    load_scene_depending_on_arguments(argc, argv);

    ImGui_ImplGlfwGL3_Init(window, true);

    set_styles();

    // why after this init?
    glfwSetKeyCallback(window, processInput);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    // glfwFocusWindow(window);

    while(!glfwWindowShouldClose(window))
    {
        controller->update();


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
void set_styles()
{
    ImGuiStyle *style = &ImGui::GetStyle();

    style->WindowPadding = ImVec2(15, 15);
    style->WindowRounding = 5.0f;
    style->FramePadding = ImVec2(5, 5);
    style->FrameRounding = 4.0f;
    style->ItemSpacing = ImVec2(12, 8);
    style->ItemInnerSpacing = ImVec2(8, 6);
    style->IndentSpacing = 25.0f;
    style->ScrollbarSize = 15.0f;
    style->ScrollbarRounding = 9.0f;
    style->GrabMinSize = 5.0f;
    style->GrabRounding = 3.0f;
/*
    style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
    style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 0.50f);
    style->Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
    style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
    style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
    style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_ComboBg] = ImVec4(0.19f, 0.18f, 0.21f, 1.00f);
    style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
    style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
    style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_Column] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_ColumnHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
    style->Colors[ImGuiCol_ColumnActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
    style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
    style->Colors[ImGuiCol_CloseButton] = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
    style->Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
    style->Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
    style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
    style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
    style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
    style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
    style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
    style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
*/
    ImGuiIO io = ImGui::GetIO();

    // io.Fonts->AddFontFromFileTTF((std::string(LAMURE_FONTS_DIR) + "/Roboto-Medium.ttf").c_str(), 12);
    // io.Fonts->AddFontFromFileTTF((std::string(LAMURE_FONTS_DIR) + "/Roboto-Medium.ttf").c_str(), 14);
    // io.Fonts->AddFontFromFileTTF((std::string(LAMURE_FONTS_DIR) + "/Roboto-Medium.ttf").c_str(), 10);
    io.Fonts->AddFontFromFileTTF((std::string(LAMURE_FONTS_DIR) + "/Roboto-Medium.ttf").c_str(), 18);
}

// void glut_mouse_movement(int x, int y) { controller->handle_mouse_movement(x, y); }
// // void glut_mouse_movement_passive(int x, int y)
// // {
// //     controller->handle_mouse_movement_passive(x, y);
// //     glutWarpPointer(960, 540);
// // }
// void mouse_callback(int button, int state, int x, int y) { controller->handle_mouse_click(button, state, x, y); }

// void glut_idle() { glutPostRedisplay(); }
