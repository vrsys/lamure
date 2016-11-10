// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "lamure/pvs/management.h"
#include <set>
#include <ctime>
#include <algorithm>
#include <fstream>
#include <lamure/ren/bvh.h>
#include <sstream>

#include <chrono>
#include <thread>

//#define ALLOW_INPUT

namespace lamure
{
namespace pvs
{

management::
management(std::vector<std::string> const& model_filenames,
    std::vector<scm::math::mat4f> const& model_transformations,
    std::set<lamure::model_t> const& visible_set,
    std::set<lamure::model_t> const& invisible_set)
    :   renderer_(nullptr),
        model_filenames_(model_filenames),
        model_transformations_(model_transformations),

        test_send_rendered_(true),
        active_camera_(nullptr),
        num_models_(0),
        dispatch_(true),
        error_threshold_(LAMURE_DEFAULT_THRESHOLD),
        near_plane_(0.001f),
        far_plane_(1000.f),
        importance_(1.f)

{
    visibility_threshold_ = 0.0001f;
    first_frame_ = true;

    lamure::ren::model_database* database = lamure::ren::model_database::get_instance();

#ifdef LAMURE_RENDERING_ENABLE_LAZY_MODELS_TEST
    assert(model_filenames_.size() > 0);
    database->add_model(model_filenames_[0], std::to_string(num_models_));
    ++num_models_;
#else
    for (const auto& filename : model_filenames_)
    {
        database->add_model(filename, std::to_string(num_models_));
        ++num_models_;
    }
#endif

    float scene_diameter = far_plane_;
    for (lamure::model_t model_id = 0; model_id < database->num_models(); ++model_id)
    {
        const auto& bb = database->get_model(model_id)->get_bvh()->get_bounding_boxes()[0];
        scene_diameter = std::max(scm::math::length(bb.max_vertex()-bb.min_vertex()), scene_diameter);
        model_transformations_[model_id] = model_transformations_[model_id] * scm::math::make_translation(database->get_model(model_id)->get_bvh()->get_translation());
    }
    far_plane_ = 2.0f * scene_diameter;

    auto root_bb = database->get_model(0)->get_bvh()->get_bounding_boxes()[0];
    scm::math::vec3 center = model_transformations_[0] * root_bb.center();
    scm::math::mat4f reset_matrix = scm::math::make_look_at_matrix(center+scm::math::vec3f(0.f, 0.1f,-0.01f), center, scm::math::vec3f(0.f, 1.f,0.f));
    float reset_diameter = scm::math::length(root_bb.max_vertex()-root_bb.min_vertex());


    std::cout << "model center : " << center << std::endl;
    std::cout << "model size : " << reset_diameter << std::endl;

    active_camera_ = new lamure::ren::camera(0, reset_matrix, reset_diameter, false, false);

    // Increase camera movement speed for debugging purpose.
    active_camera_->set_dolly_sens_(20.5f);

    // Set camera view manually for debug purpose.
    active_camera_->set_view_matrix(scm::math::mat4d(0.05, -0.3, 0.95, 0.0,
                                            1.0, -0.09, 0.03, 0.0,
                                            0.07, 0.95, 0.32, 0.0,
                                            -75.0, 4.75, -173.4, 1.0));

    renderer_ = new Renderer(model_transformations_, visible_set, invisible_set);
}

management::
~management()
{
    if (active_camera_ != nullptr)
    {
        delete active_camera_;
        active_camera_ = nullptr;
    }
    if (renderer_ != nullptr)
    {
        delete renderer_;
        renderer_ = nullptr;
    }
}

bool management::
MainLoop()
{
    lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
    lamure::ren::controller* controller = lamure::ren::controller::get_instance();
    lamure::ren::cut_database* cuts = lamure::ren::cut_database::get_instance();

    bool signal_shutdown = false;

    controller->reset_system();

    lamure::context_t context_id = controller->deduce_context_id(0);
    lamure::view_t view_id = controller->deduce_view_id(context_id, active_camera_->view_id());
    
    bool done = false;
    int repetition_counter = 0;

    if(first_frame_)
    {
        controller->dispatch(context_id, renderer_->device());
        first_frame_ = false;
    }
    else
    {
        std::vector<int> old_cut_lengths(num_models_, -1);

        while (!done)
        {
            // Cut update runs asynchronous, so wait until it is done.
            if (!controller->is_cut_update_in_progress(context_id))
            {
                bool length_changed = false;

                for (lamure::model_t model_index = 0; model_index < num_models_; ++model_index)
                {
                    lamure::model_t model_id = controller->deduce_model_id(std::to_string(model_index));

                    // Check if the cut length changed in comparison to previous frame.
                    lamure::ren::cut& cut = cuts->get_cut(context_id, view_id, model_id);
                    if(cut.complete_set().size() > old_cut_lengths[model_index])
                    {
                        length_changed = true;
                    }
                    //std::cout << "model: " << model_id << "  new length: " << cut.complete_set().size() << "  old length: " << old_cut_lengths[model_index] << std::endl;
                    old_cut_lengths[model_index] = cut.complete_set().size();

                    cuts->send_transform(context_id, model_id, model_transformations_[model_id]);
                    cuts->send_threshold(context_id, model_id, error_threshold_ / importance_);

                    //send rendered, threshold, camera, 
                    cuts->send_rendered(context_id, model_id);
                    database->get_model(model_id)->set_transform(model_transformations_[model_id]);

                    lamure::view_t cam_id = controller->deduce_view_id(context_id, active_camera_->view_id());
                    cuts->send_camera(context_id, cam_id, *active_camera_);

                    std::vector<scm::math::vec3d> corner_values = active_camera_->get_frustum_corners();
                    double top_minus_bottom = scm::math::length((corner_values[2]) - (corner_values[0]));
                    float height_divided_by_top_minus_bottom = lamure::ren::policy::get_instance()->window_height() / top_minus_bottom;

                    cuts->send_height_divided_by_top_minus_bottom(context_id, cam_id, height_divided_by_top_minus_bottom);
                }

                controller->dispatch(context_id, renderer_->device());

                // Stop if no length change was detected.
                if(!length_changed)
                {
                    ++repetition_counter;

                    if(repetition_counter >= 10)
                    {
                        done = true;
                    }
                }
                else
                {
                    repetition_counter = 0;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    renderer_->set_radius_scale(importance_);
    renderer_->render(context_id, *active_camera_, view_id, controller->get_context_memory(context_id, lamure::ren::bvh::primitive_type::POINTCLOUD, renderer_->device()), 0);

    //renderer_->display_status("");
    // Output current view matrix for debug purpose.
    std::stringstream cam_mat_string;
    cam_mat_string << "visibility threshold: " << visibility_threshold_ << std::endl;

    cam_mat_string << "view matrix:\n";
    for(int index = 0; index < 16; ++index)
    {
        cam_mat_string << active_camera_->get_view_matrix()[index] << "   ";
        if((index + 1) % 4 == 0)
        {
            cam_mat_string << "\n";
        }
    }
    renderer_->display_status(cam_mat_string.str());

    if(done)
    {
        id_histogram hist = renderer_->create_node_id_histogram(true);
        renderer_->compare_histogram_to_cut(hist, visibility_threshold_, false);
        signal_shutdown = true;
    }

/*#ifdef LAMURE_CUT_UPDATE_ENABLE_MEASURE_SYSTEM_PERFORMANCE
    system_performance_timer_.stop();
    boost::timer::cpu_times const elapsed_times(system_performance_timer_.elapsed());
    boost::timer::nanosecond_type const elapsed(elapsed_times.system + elapsed_times.user);

    if (elapsed >= boost::timer::nanosecond_type(1.0f * 1000 * 1000 * 1000)) //1 second
    {
       boost::timer::cpu_times const result_elapsed_times(system_result_timer_.elapsed());
       boost::timer::nanosecond_type const result_elapsed(result_elapsed_times.system + result_elapsed_times.user);


       std::cout << "no cut update after " << result_elapsed/(1000 * 1000 * 1000) << " seconds" << std::endl;
       system_performance_timer_.start();
    }
    else
    {
       system_performance_timer_.resume();
    }

#endif*/

    return signal_shutdown;
}

void management::
update_trackball(int x, int y)
{
#ifdef ALLOW_INPUT
    active_camera_->update_trackball(x,y, width_, height_, mouse_state_);
#endif
}

void management::
RegisterMousePresses(int button, int state, int x, int y)
{
#ifdef ALLOW_INPUT
    switch (button) {
        case GLUT_LEFT_BUTTON:
            {
                mouse_state_.lb_down_ = (state == GLUT_DOWN) ? true : false;
            }break;
        case GLUT_MIDDLE_BUTTON:
            {
                mouse_state_.mb_down_ = (state == GLUT_DOWN) ? true : false;
            }break;
        case GLUT_RIGHT_BUTTON:
            {
                mouse_state_.rb_down_ = (state == GLUT_DOWN) ? true : false;
            }break;
    }

    float trackball_init_x = 2.f * float(x - (width_/2))/float(width_) ;
    float trackball_init_y = 2.f * float(height_ - y - (height_/2))/float(height_);

    active_camera_->update_trackball_mouse_pos(trackball_init_x, trackball_init_y);
#endif
}

void management::
dispatchKeyboardInput(unsigned char key)
{
#ifdef ALLOW_INPUT
    switch(key)
    {
        case 's':
        {
            id_histogram hist = renderer_->create_node_id_histogram(false);
            renderer_->compare_histogram_to_cut(hist, visibility_threshold_, true);
            break;
        }

        // Reset node visibility (all nodess become visible again).
        case 'a':
        {
            lamure::ren::cut_database* cuts = lamure::ren::cut_database::get_instance();
            lamure::ren::model_database* database = lamure::ren::model_database::get_instance();

            lamure::context_t context_id = 0;
            lamure::view_t view_id = 0;

            for(lamure::model_t model_id = 0; model_id < database->num_models(); ++model_id)
            {
                lamure::ren::cut& cut = cuts->get_cut(context_id, view_id, model_id);
                std::vector<lamure::ren::cut::node_slot_aggregate> renderable = cut.complete_set();

                for(unsigned int index = 0; index < renderable.size(); ++index)
                {
                    lamure::ren::cut::node_slot_aggregate& aggregate = renderable.at(index);
                    database->get_model(model_id)->get_bvh()->set_visibility(aggregate.node_id_, lamure::ren::bvh::node_visibility::NODE_VISIBLE);
                }
            }
            break;
        }

        case 'e':
            visibility_threshold_ *= 1.1f;
            break;

        case 'd':
            visibility_threshold_ /= 1.1f;
            break;

        case 'q':
            Toggledispatching();
            break;

        case 'w':
            renderer_->toggle_bounding_box_rendering();
            break;
    }
#endif
}

void management::
dispatchResize(int w, int h)
{
    width_ = w;
    height_ = h;

    renderer_->reset_viewport(w,h);

    lamure::ren::policy* policy = lamure::ren::policy::get_instance();
    policy->set_window_width(w);
    policy->set_window_height(h);

    active_camera_->set_projection_matrix(30.0f, float(w)/float(h),  near_plane_, far_plane_);
}

void management::
Toggledispatching()
{
    dispatch_ = ! dispatch_;
}

void management::
DecreaseErrorThreshold()
{
    error_threshold_ -= 0.1f;
    if (error_threshold_ < LAMURE_MIN_THRESHOLD)
        error_threshold_ = LAMURE_MIN_THRESHOLD;
}

void management::
IncreaseErrorThreshold()
{
    error_threshold_ += 0.1f;
    if (error_threshold_ > LAMURE_MAX_THRESHOLD)
        error_threshold_ = LAMURE_MAX_THRESHOLD;
}

}
}
