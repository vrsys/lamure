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

#include "lamure/pvs/pvs_database.h"

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
    update_position_for_pvs_ = true;

#ifdef LAMURE_PVS_MEASURE_PERFORMANCE
    total_cut_update_time_ = 0.0;
    total_render_time_ = 0.0;
    total_histogram_evaluation_time_ = 0.0;
#endif

#ifndef LAMURE_PVS_USE_AS_RENDERER
    lamure::pvs::pvs_database::get_instance()->activate(false);
#endif

    lamure::ren::model_database* database = lamure::ren::model_database::get_instance();

    for (const auto& filename : model_filenames_)
    {
        database->add_model(filename, std::to_string(num_models_));
        ++num_models_;
    }

    total_depth_rendered_nodes_.resize(num_models_);
    total_num_rendered_nodes_.resize(num_models_);

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

            near_plane = current_cell->get_size().x * 0.5f;
            break;

        case 1:
            look_dir = scm::math::vec3d(-1.0, 0.0, 0.0);

            near_plane = current_cell->get_size().x * 0.5f;
            break;

        case 2:
            look_dir = scm::math::vec3d(0.0, 1.0, 0.0);
            up_dir = scm::math::vec3d(0.0, 0.0, 1.0);

            near_plane = current_cell->get_size().y * 0.5f;
            break;

        case 3:
            look_dir = scm::math::vec3d(0.0, -1.0, 0.0);
            up_dir = scm::math::vec3d(0.0, 0.0, 1.0);

            near_plane = current_cell->get_size().y * 0.5f;
            break;

        case 4:
            look_dir = scm::math::vec3d(0.0, 0.0, 1.0);
            
            near_plane = current_cell->get_size().z * 0.5f;
            break;

        case 5:
            look_dir = scm::math::vec3d(0.0, 0.0, -1.0);

            near_plane = current_cell->get_size().z * 0.5f;
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
    total_cut_update_time_ += elapsed_seconds.count();
#endif
#endif

#ifdef LAMURE_PVS_USE_AS_RENDERER
    // Update PVS database with current camera position before rendering.
    pvs_database* pvs = pvs_database::get_instance();
    if(update_position_for_pvs_)
    {
        scm::math::mat4f cm = scm::math::inverse(scm::math::mat4f(active_camera_->trackball_matrix()));
        scm::math::vec3d cam_pos = scm::math::vec3d(cm[12], cm[13], cm[14]);
        pvs->set_viewer_position(cam_pos);
    }
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
    total_render_time_ += elapsed_seconds.count();
#endif

#ifdef LAMURE_PVS_USE_AS_RENDERER
    // Output current view matrix for debug purpose.
    std::stringstream add_info_string;
    add_info_string << "visibility threshold: " << visibility_threshold_ << std::endl;

    add_info_string << "camera position:\n";
    scm::math::mat4f cm = scm::math::inverse(scm::math::mat4f(active_camera_->trackball_matrix()));
    for(int index = 11; index < 15; ++index)
    {
        add_info_string << cm[index] << "  ";
    }
    add_info_string << std::endl;

    add_info_string << "use PVS: " << pvs->is_activated() << std::endl;
    add_info_string << "update pos: " << update_position_for_pvs_ << std::endl;

    renderer_->display_status(add_info_string.str());
    //renderer_->display_status("");
#endif

#ifndef LAMURE_PVS_USE_AS_RENDERER
    if(!first_frame_)
    {
        // Analyze histogram data of current rendered image.
        if(renderer_->get_rendered_node_count() > 0)
        {
        #ifdef LAMURE_PVS_MEASURE_PERFORMANCE
            // Measure histogram creation performance.
            start_time = std::chrono::system_clock::now();
        #endif

            id_histogram hist = renderer_->create_node_id_histogram(false, (direction_counter_ * visibility_grid_->get_cell_count()) + current_grid_index_);
            std::map<model_t, std::vector<node_t>> visible_ids = hist.get_visible_nodes(width_ * height_, visibility_threshold_);

            for(std::map<model_t, std::vector<node_t>>::iterator iter = visible_ids.begin(); iter != visible_ids.end(); ++iter)
            {
                model_t model_id = iter->first;

                for(node_t node_id : iter->second)
                {
                    current_cell->set_visibility(model_id, node_id, true);
                }
            }

        #ifdef LAMURE_PVS_MEASURE_PERFORMANCE
            end_time = std::chrono::system_clock::now();
            elapsed_seconds = end_time - start_time;
            total_histogram_evaluation_time_ += elapsed_seconds.count();
        #endif 
        }

        // Collect data to calculate average depth of nodes per model.
        for (lamure::model_t model_index = 0; model_index < num_models_; ++model_index)
        {
            lamure::model_t model_id = controller->deduce_model_id(std::to_string(model_index));
            lamure::ren::cut& cut = cuts->get_cut(context_id, view_id, model_id);
            std::vector<lamure::ren::cut::node_slot_aggregate> renderable = cut.complete_set();

            // Count nodes in the current cut.
            for(auto const& node_slot_aggregate : renderable)
            {
                total_depth_rendered_nodes_[model_index][current_grid_index_] = total_depth_rendered_nodes_[model_index][current_grid_index_] + database->get_model(model_id)->get_bvh()->get_depth_of_node(node_slot_aggregate.node_id_);
                total_num_rendered_nodes_[model_index][current_grid_index_] = total_num_rendered_nodes_[model_index][current_grid_index_] + 1;
            }
        }

        current_grid_index_++;
        if(current_grid_index_ == visibility_grid_->get_cell_count())
        {
            current_grid_index_ = 0;
            direction_counter_++;

            if(direction_counter_ == 6)
            {
                signal_shutdown = true;
            }
        }
    }
#endif

    // Once the visibility test is complete ...
    if(signal_shutdown)
    {
    #ifdef LAMURE_PVS_MEASURE_PERFORMANCE
        start_time = std::chrono::system_clock::now();
    #endif

        // ... calculate which nodes are inside the view cells based on the average depth of the nodes inside the cuts during rendering.
        std::cout << "start check for nodes inside grid cells..." << std::endl;
        check_for_nodes_within_cells(total_depth_rendered_nodes_, total_num_rendered_nodes_);
        std::cout << "node check finished" << std::endl;

    #ifdef LAMURE_PVS_MEASURE_PERFORMANCE
        end_time = std::chrono::system_clock::now();
        elapsed_seconds = end_time - start_time;
        double node_within_cell_check_time = elapsed_seconds.count();
        start_time = std::chrono::system_clock::now();
    #endif

        // ... set visibility of LOD-trees based on rendered nodes.
        std::cout << "start visibility propagation..." << std::endl;
        emit_node_visibility(visibility_grid_);
        std::cout << "visibility propagation finished" << std::endl;

    #ifdef LAMURE_PVS_MEASURE_PERFORMANCE
        end_time = std::chrono::system_clock::now();
        elapsed_seconds = end_time - start_time;
        double visibility_propagation_time = elapsed_seconds.count();

        std::cout << "\n---------- average performance in seconds ----------" << std::endl;
        std::cout << "cut update: " << total_cut_update_time_ / (6 * visibility_grid_->get_cell_count()) << std::endl;
        std::cout << "rendering: " << total_render_time_ / (6 * visibility_grid_->get_cell_count()) << std::endl;
        std::cout << "histogram evaluation: " << total_histogram_evaluation_time_ / (6 * visibility_grid_->get_cell_count()) << std::endl;

        std::cout << "\n---------- total performance in seconds ----------" << std::endl;
        std::cout << "cut update: " << total_cut_update_time_ << std::endl;
        std::cout << "rendering: " << total_render_time_ << std::endl;
        std::cout << "histogram evaluation: " << total_histogram_evaluation_time_ << std::endl;
        std::cout << "node in cell check: " << node_within_cell_check_time << std::endl;
        std::cout << "visibility propagation: " << visibility_propagation_time << std::endl;
        std::cout << std::endl;
    #endif

    #ifdef LAMURE_PVS_MEASURE_VISIBILITY
        // Write collected visibility data to file.
        save_visibility_data();
    #endif
    }

    return signal_shutdown;
}

void management::
check_for_nodes_within_cells(const std::vector<std::vector<size_t>>& total_depths, const std::vector<std::vector<size_t>>& total_nums)
{
    lamure::ren::model_database* database = lamure::ren::model_database::get_instance();

    for(model_t model_index = 0; model_index < database->num_models(); ++model_index)
    {
        for(size_t cell_index = 0; cell_index < visibility_grid_->get_cell_count(); ++cell_index)
        {
            // Create bounding box of view cell.
            view_cell* current_cell = visibility_grid_->get_cell_at_index(cell_index);
                
            vec3r min_vertex(current_cell->get_position_center() - (current_cell->get_size() * 0.5f));
            vec3r max_vertex(current_cell->get_position_center() + (current_cell->get_size() * 0.5f));
            bounding_box cell_bounds(min_vertex, max_vertex);

            // We can get the first and last index of the nodes on a certain depth inside the bvh.
            unsigned int average_depth = total_depth_rendered_nodes_[model_index][cell_index] / total_num_rendered_nodes_[model_index][cell_index];

            node_t start_index = database->get_model(model_index)->get_bvh()->get_first_node_id_of_depth(average_depth);
            node_t end_index = start_index + database->get_model(model_index)->get_bvh()->get_length_of_depth(average_depth);

            for(node_t node_index = start_index; node_index < end_index; ++node_index)
            {
                // Create bounding box of node.
                scm::gl::boxf node_bounding_box = database->get_model(model_index)->get_bvh()->get_bounding_boxes()[node_index];
                bounding_box node_bounds(vec3r(node_bounding_box.min_vertex()), vec3r(node_bounding_box.max_vertex()));

                // check if the bounding boxes collide.
                if(cell_bounds.intersects(node_bounds))
                {
                    current_cell->set_visibility(model_index, node_index, true);
                }
            }
        }
    }
}

void management::
emit_node_visibility(grid* visibility_grid)
{
    // Advance node visibility downwards and upwards in the LOD-hierarchy.
    // Since only a single LOD-level was rendered in the visibility test, this is necessary to produce a complete PVS.
    #pragma omp parallel for
    for(size_t cell_index = 0; cell_index < visibility_grid->get_cell_count(); ++cell_index)
    {
        view_cell* current_cell = visibility_grid->get_cell_at_index(cell_index);
        std::map<model_t, std::vector<node_t>> visible_indices = current_cell->get_visible_indices();

        for(std::map<model_t, std::vector<node_t>>::const_iterator map_iter = visible_indices.begin(); map_iter != visible_indices.end(); ++map_iter)
        {
            for(node_t node_index = 0; node_index < map_iter->second.size(); ++node_index)
            {
                node_t visible_node_id = map_iter->second.at(node_index);

                // Communicate visibility to children and parents nodes of visible nodes.
                set_node_children_visible(map_iter->first, visible_node_id, current_cell);
                set_node_parents_visible(map_iter->first, visible_node_id, current_cell);
            }       
        }
    }
}

void management::
set_node_parents_visible(const model_t& model_id, const node_t& node_id, view_cell* cell)
{
    // Set parents of a visible node visible, too.
    // Necessary since only a single LOD-level is rendered during the visibility test.
    lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
    node_t parent_id = database->get_model(model_id)->get_bvh()->get_parent_id(node_id);

    if(parent_id != lamure::invalid_node_t && !cell->get_visibility(model_id, parent_id))
    {
        if(cell != nullptr)
        {
            cell->set_visibility(model_id, parent_id, true);
        }
        set_node_parents_visible(model_id, parent_id, cell);
    }
}

void management::
set_node_children_visible(const model_t& model_id, const node_t& node_id, view_cell* cell)
{
    // Set children of a visible node visible, too.
    lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
    uint32_t fan_factor = database->get_model(model_id)->get_bvh()->get_fan_factor();

    for(uint32_t child_index = 0; child_index < fan_factor; ++child_index)
    {
        node_t child_id = database->get_model(model_id)->get_bvh()->get_child_id(node_id, child_index);
        if(child_id < database->get_model(model_id)->get_bvh()->get_num_nodes() && !cell->get_visibility(model_id, child_id))
        {
            if(cell != nullptr)
            {
                cell->set_visibility(model_id, child_id, true);
            }
            set_node_children_visible(model_id, child_id, cell);
        }
    } 
}

#ifdef LAMURE_PVS_MEASURE_VISIBILITY
void management::
save_visibility_data()
{
    lamure::ren::model_database* database = lamure::ren::model_database::get_instance();

    std::ofstream file_out;
    file_out.open("/home/tiwo9285/test_visibility.txt");

    node_t absolute_visible_nodes = 0;
    node_t absolute_num_nodes = 0;

    for(size_t cell_index = 0; cell_index < visibility_grid_->get_cell_count(); ++cell_index)
    {
        size_t total_visible_nodes = 0;
        size_t total_num_nodes = 0;
        std::map<model_t, std::vector<node_t>> visible_indices = visibility_grid_->get_cell_at_index_const(cell_index)->get_visible_indices();

        for(model_t model_index = 0; model_index < num_models_; ++model_index)
        {
            node_t visible_nodes = visible_indices[model_index].size();
            node_t num_nodes = database->get_model(model_index)->get_bvh()->get_num_nodes();

            total_visible_nodes += visible_nodes;
            total_num_nodes += num_nodes;

            float occlusion = 100.0f - ((float)visible_nodes / (float)num_nodes) * 100.0f;
            file_out << "cell: " << cell_index << "   model: " << model_index << "   occlusion: " << occlusion << "   nodes: " << visible_nodes << "/" << num_nodes << std::endl;
        }

        float occlusion = 100.0f - ((float)total_visible_nodes / (float)total_num_nodes) * 100.0f;
        file_out << "cell: " << cell_index << "   total occlusion: " << occlusion << "   nodes: " << total_visible_nodes << "/" << total_num_nodes << std::endl << std::endl;

        absolute_visible_nodes += total_visible_nodes;
        absolute_num_nodes += total_num_nodes;
    }

    float occlusion = (1.0f - (float)absolute_visible_nodes / (float)absolute_num_nodes) * 100.0f;
    file_out << "absolute occlusion: " << occlusion << "   nodes: " << absolute_visible_nodes << "/" << absolute_num_nodes << std::endl << std::endl;

    file_out.close();
}
#endif

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
    switch (button)
    {
        case GLUT_LEFT_BUTTON:
            {
                mouse_state_.lb_down_ = (state == GLUT_DOWN) ? true : false;
            }
            break;
        case GLUT_MIDDLE_BUTTON:
            {
                mouse_state_.mb_down_ = (state == GLUT_DOWN) ? true : false;
            }
            break;
        case GLUT_RIGHT_BUTTON:
            {
                mouse_state_.rb_down_ = (state == GLUT_DOWN) ? true : false;
            }
            break;
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
            renderer_->compare_histogram_to_cut(hist, visibility_threshold_);
            break;
        }

        case 'p':
        {
            pvs_database::get_instance()->activate(!pvs_database::get_instance()->is_activated());
            break;
        }

        case 'o':
        {
            update_position_for_pvs_ = !update_position_for_pvs_;
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

    // Set size of containers used to collect data on rendered node depth.
    if(visibility_grid_ != nullptr)
    {
        total_depth_rendered_nodes_.clear();
        total_num_rendered_nodes_.clear();

        for(model_t model_index = 0; model_index < num_models_; ++model_index)
        {
            total_depth_rendered_nodes_[model_index].resize(visibility_grid_->get_cell_count());
            total_num_rendered_nodes_[model_index].resize(visibility_grid_->get_cell_count());
        }
    }
}

}
}
