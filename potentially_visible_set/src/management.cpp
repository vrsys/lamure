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
//#define LAMURE_PVS_USE_AS_RENDERER
//#define LAMURE_PVS_MEASURE_PERFORMANCE

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
    visibility_grid_ = nullptr;
    current_grid_index_ = 0;
    direction_counter_ = 0;

#ifdef LAMURE_PVS_MEASURE_PERFORMANCE
    average_cut_update_time_ = 0.0;
    average_render_time_ = 0.0;
#endif

    lamure::ren::model_database* database = lamure::ren::model_database::get_instance();

    for (const auto& filename : model_filenames_)
    {
        database->add_model(filename, std::to_string(num_models_));
        ++num_models_;
    }

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
    scm::math::mat4f reset_matrix = scm::math::make_look_at_matrix(center + scm::math::vec3f(0.0f, 0.0f, 0.1f), center, scm::math::vec3f(0.0f, 1.0f,0.0f));
    float reset_diameter = scm::math::length(root_bb.max_vertex()-root_bb.min_vertex());

    std::cout << "model center : " << center << std::endl;
    std::cout << "model size : " << reset_diameter << std::endl;

    active_camera_ = new lamure::ren::camera(0, reset_matrix, reset_diameter, false, false);

    // Increase camera movement speed for debugging purpose.
    active_camera_->set_dolly_sens_(20.5f);

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
    
#ifndef LAMURE_PVS_USE_AS_RENDERER
    int repetition_counter = 0;
    view_cell* current_cell = visibility_grid_->get_cell_at_index(current_grid_index_);

    scm::math::vec3d look_dir;
    scm::math::vec3d up_dir(0.0, 1.0, 0.0);
    
    float opening_angle = 90.0f;        // TODO: these two should also be computed per cell (these constants only work in the regular box case)
    float aspect_ratio = 1.0f;
    float near_plane = 0.01f;

    switch(direction_counter_)
    {
        case 0:
            look_dir = scm::math::vec3d(1.0, 0.0, 0.0);

            //near_plane = current_cell->get_size().x * 0.5f;
            break;

        case 1:
            look_dir = scm::math::vec3d(-1.0, 0.0, 0.0);

            //near_plane = current_cell->get_size().x * 0.5f;
            break;

        case 2:
            look_dir = scm::math::vec3d(0.0, 1.0, 0.0);
            up_dir = scm::math::vec3d(0.0, 0.0, 1.0);

            //near_plane = current_cell->get_size().y * 0.5f;
            break;

        case 3:
            look_dir = scm::math::vec3d(0.0, -1.0, 0.0);
            up_dir = scm::math::vec3d(0.0, 0.0, 1.0);

            //near_plane = current_cell->get_size().y * 0.5f;
            break;

        case 4:
            look_dir = scm::math::vec3d(0.0, 0.0, 1.0);
            
            //near_plane = current_cell->get_size().z * 0.5f;
            break;

        case 5:
            look_dir = scm::math::vec3d(0.0, 0.0, -1.0);

            //near_plane = current_cell->get_size().z * 0.5f;
            break;
            
        default:
            break;
    }

    active_camera_->set_projection_matrix(opening_angle, aspect_ratio, near_plane, far_plane_);
    active_camera_->set_view_matrix(scm::math::make_look_at_matrix(current_cell->get_position_center(), current_cell->get_position_center() + look_dir, up_dir));  // look_at(eye, center, up)

#ifdef LAMURE_PVS_MEASURE_PERFORMANCE
    // Performance measurement of cut update.
    std::chrono::time_point<std::chrono::system_clock> start_time, end_time;
    start_time = std::chrono::system_clock::now();
#endif

    if(first_frame_)
    {
        controller->dispatch(context_id, renderer_->device());
        first_frame_ = false;
    }
    else
    {
        std::vector<unsigned int> old_cut_lengths(num_models_, 0);
        bool done = false;

        while (!done)
        {
            // Cut update runs asynchronous, so wait until it is done.
            if (!controller->is_cut_update_in_progress(context_id))
            {
                bool length_changed = false;
#endif

                for (lamure::model_t model_index = 0; model_index < num_models_; ++model_index)
                {
                    lamure::model_t model_id = controller->deduce_model_id(std::to_string(model_index));

                #ifndef LAMURE_PVS_USE_AS_RENDERER
                    // Check if the cut length changed in comparison to previous frame.
                    lamure::ren::cut& cut = cuts->get_cut(context_id, view_id, model_id);
                    if(cut.complete_set().size() > old_cut_lengths[model_index])
                    {
                        length_changed = true;
                    }
                    old_cut_lengths[model_index] = cut.complete_set().size();
                #endif

                    cuts->send_transform(context_id, model_id, model_transformations_[model_id]);
                    cuts->send_threshold(context_id, model_id, error_threshold_ / importance_);

                    // Send rendered, threshold, camera, ... 
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

#ifndef LAMURE_PVS_USE_AS_RENDERER
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

            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }

#ifdef LAMURE_PVS_MEASURE_PERFORMANCE
    end_time = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_time - start_time;
    std::cout << "cut update time: " << elapsed_seconds.count() << std::endl;
    average_cut_update_time_ += elapsed_seconds.count();
#endif
#endif

#ifdef LAMURE_PVS_MEASURE_PERFORMANCE
    // Measure rendering performance.
    start_time = std::chrono::system_clock::now();
#endif

    renderer_->set_radius_scale(importance_);
    renderer_->render(context_id, *active_camera_, view_id, controller->get_context_memory(context_id, lamure::ren::bvh::primitive_type::POINTCLOUD, renderer_->device()), 0);

#ifdef LAMURE_PVS_MEASURE_PERFORMANCE
    end_time = std::chrono::system_clock::now();
    elapsed_seconds = end_time - start_time;
    std::cout << "render time: " << elapsed_seconds.count() << std::endl;
    average_render_time_ += elapsed_seconds.count();
#endif

    // Debug output of current state.
    //std::cout << "rendered view cell: " << current_grid_index_ << "  direction: " << direction_counter_ << std::endl;

#ifdef LAMURE_PVS_USE_AS_RENDERER
    // Output current view matrix for debug purpose.
    std::stringstream cam_mat_string;
    cam_mat_string << "visibility threshold: " << visibility_threshold_ << std::endl;
    scm::math::mat4f view_mat = scm::math::transpose(active_camera_->get_view_matrix());

    cam_mat_string << "view matrix:\n";
    for(int index = 0; index < 16; ++index)
    {
        cam_mat_string << view_mat[index] << "   ";
        if((index + 1) % 4 == 0)
        {
            cam_mat_string << "\n";
        }
    }
    renderer_->display_status(cam_mat_string.str());
    //renderer_->display_status("");
#endif

#ifndef LAMURE_PVS_USE_AS_RENDERER
    if(!first_frame_)
    {
        if(renderer_->get_rendered_node_count() > 0)
        {
        #ifdef LAMURE_PVS_MEASURE_PERFORMANCE
            // Measure histogram creation performance.
            start_time = std::chrono::system_clock::now();
        #endif

            id_histogram hist = renderer_->create_node_id_histogram(false, (direction_counter_ * visibility_grid_->get_cell_count()) + current_grid_index_);
            std::map<unsigned int, std::vector<unsigned int>> visible_ids = hist.get_visible_nodes(width_ * height_, visibility_threshold_);

            for(std::map<unsigned int, std::vector<unsigned int>>::iterator iter = visible_ids.begin(); iter != visible_ids.end(); ++iter)
            {
                for(unsigned int node_id : iter->second)
                {
                    current_cell->set_visibility(iter->first, node_id);
                }
            }
            //renderer_->compare_histogram_to_cut(hist, visibility_threshold_, false);

        #ifdef LAMURE_PVS_MEASURE_PERFORMANCE
            end_time = std::chrono::system_clock::now();
            elapsed_seconds = end_time - start_time;
            std::cout << "histogram creation time: " << elapsed_seconds.count() << std::endl;
        #endif 
        }

        current_grid_index_++;
        if(current_grid_index_ == visibility_grid_->get_cell_count())
        {
            current_grid_index_ = 0;
            direction_counter_++;

            if(direction_counter_ == 6)
            {
                signal_shutdown = true;

            #ifdef LAMURE_PVS_MEASURE_PERFORMANCE
                std::cout << "---------- average performance in seconds ----------" << std::endl;
                std::cout << "cut update: " << average_cut_update_time_ / (6 * visibility_grid_->get_cell_count()) << std::endl;
                std::cout << "rendering: " << average_render_time_ / (6 * visibility_grid_->get_cell_count()) << std::endl;
            #endif
            }
        }
    }
#endif
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
            id_histogram hist = renderer_->create_node_id_histogram(false, 0);
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
    {
        error_threshold_ = LAMURE_MIN_THRESHOLD;
    }
}

void management::
IncreaseErrorThreshold()
{
    error_threshold_ += 0.1f;
    if (error_threshold_ > LAMURE_MAX_THRESHOLD)
    {
        error_threshold_ = LAMURE_MAX_THRESHOLD;
    }
}

void management::
set_grid(grid* visibility_grid)
{
    visibility_grid_ = visibility_grid;
}

}
}
