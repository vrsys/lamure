// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_APP_MANAGEMENT_H_
#define REN_APP_MANAGEMENT_H_

#include <lamure/utils.h>
#include <lamure/types.h>

#include "split_screen_renderer.h"
#include "renderer.h"
#include <lamure/ren/config.h>

#include <lamure/ren/model_database.h>
#include <lamure/ren/controller.h>
#include <lamure/ren/cut.h>
#include <lamure/ren/cut_update_pool.h>

#include <GL/freeglut.h>

#include <FreeImagePlus.h>

#include <lamure/ren/ray.h>

class Management
{
public:
                        Management(std::vector<std::string> const& model_filenames,
                            std::vector<scm::math::mat4f> const& model_transformations,
                            const std::set<lamure::model_t>& visible_set,
                            const std::set<lamure::model_t>& invisible_set);
    virtual             ~Management();

                        Management(const Management&) = delete;
                        Management& operator=(const Management&) = delete;

    void                MainLoop();
    void                UpdateTrackball(int x, int y);
    void                RegisterMousePresses(int button, int state, int x, int y);
    void                DispatchKeyboardInput(unsigned char key);
    void                DispatchResize(int w, int h);

    void                PrintInfo();
    void                SetSceneName();

    float               error_threshold_;
    void                IncreaseErrorThreshold();
    void                DecreaseErrorThreshold();

protected:

    void                ToggleDispatching();

private:

#ifdef LAMURE_RENDERING_USE_SPLIT_SCREEN
    split_screen_renderer* renderer_;
    lamure::ren::Camera*   active_camera_left_;
    lamure::ren::Camera*   active_camera_right_;
    bool                control_left_;
#endif

#ifndef LAMURE_RENDERING_USE_SPLIT_SCREEN
    renderer* renderer_;
#endif

    lamure::ren::Camera*   active_camera_;

    int32_t             width_;
    int32_t             height_;

    float importance_;

    bool test_send_rendered_;

    lamure::view_t         num_cameras_;
    std::vector<lamure::ren::Camera*> cameras_;

    lamure::ren::Camera::Mousestate mouse_state_;

    bool                fast_travel_;

    bool                dispatch_;
    bool                trigger_one_update_;

    scm::math::mat4f    reset_matrix_;
    float               reset_diameter_;

    lamure::model_t        num_models_;

    scm::math::vec3f    detail_translation_;
    float               detail_angle_;
    float               near_plane_;
    float               far_plane_;

    std::vector<scm::math::mat4f> model_transformations_;
    std::vector<std::string> model_filenames_;

#ifdef LAMURE_CUT_UPDATE_ENABLE_MEASURE_SYSTEM_PERFORMANCE
    boost::timer::cpu_timer system_performance_timer_;
    boost::timer::cpu_timer system_result_timer_;
#endif
};


#endif // REN_MANAGEMENT_H_

