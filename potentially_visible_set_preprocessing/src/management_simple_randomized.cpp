// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "lamure/pvs/management_simple_randomized.h"

#include <set>
#include <ctime>
#include <algorithm>
#include <lamure/ren/bvh.h>
#include <thread>
#include <random>
#include <fstream>

#include "lamure/pvs/pvs_database.h"
#include "lamure/pvs/grid_regular.h"

namespace lamure
{
namespace pvs
{

management_simple_randomized::
management_simple_randomized(std::vector<std::string> const& model_filenames,
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

    lamure::pvs::pvs_database::get_instance()->activate(false);

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

    renderer_ = new Renderer(model_transformations_, visible_set, invisible_set);
}

management_simple_randomized::
~management_simple_randomized()
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

bool management_simple_randomized::
MainLoop()
{
    lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
    lamure::ren::controller* controller = lamure::ren::controller::get_instance();
    lamure::ren::cut_database* cuts = lamure::ren::cut_database::get_instance();

    bool signal_shutdown = false;

    controller->reset_system();

    lamure::context_t context_id = controller->deduce_context_id(0);
    lamure::view_t view_id = controller->deduce_view_id(context_id, active_camera_->view_id());
    
    int repetition_counter = 0;
    const view_cell* current_cell = visibility_grid_->get_cell_at_index(current_grid_index_);

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
    active_camera_->set_view_matrix(scm::math::make_look_at_matrix(current_position_in_current_view_cell_, current_position_in_current_view_cell_ + look_dir, up_dir));  // look_at(eye, center, up)

    if(first_frame_)
    {
        controller->dispatch(context_id, renderer_->device());
        first_frame_ = false;

        current_test_start_time_ = std::chrono::system_clock::now();
        remaining_duration_visibility_test_in_seconds_ = duration_visibility_test_in_seconds_;

        num_samples_per_view_cell_.resize(visibility_grid_->get_cell_count());
        view_cell_distribution_ = std::uniform_int_distribution<size_t>(0, visibility_grid_->get_cell_count() - 1);
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

                for (lamure::model_t model_index = 0; model_index < num_models_; ++model_index)
                {
                    lamure::model_t model_id = controller->deduce_model_id(std::to_string(model_index));

                    // Check if the cut length changed in comparison to previous frame.
                    lamure::ren::cut& cut = cuts->get_cut(context_id, view_id, model_id);
                    if(cut.complete_set().size() > old_cut_lengths[model_index])
                    {
                        length_changed = true;
                    }
                    old_cut_lengths[model_index] = cut.complete_set().size();

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

    renderer_->set_radius_scale(importance_);
    renderer_->render(context_id, *active_camera_, view_id, controller->get_context_memory(context_id, lamure::ren::bvh::primitive_type::POINTCLOUD, renderer_->device()), 0);

    if(!first_frame_)
    {
        // Analyze histogram data of current rendered image.
        if(renderer_->get_rendered_node_count() > 0)
        {
            id_histogram hist = renderer_->create_node_id_histogram(false, (direction_counter_ * visibility_grid_->get_cell_count()) + current_grid_index_);
            std::map<model_t, std::vector<node_t>> visible_ids = hist.get_visible_nodes(width_ * height_, visibility_threshold_);

            for(std::map<model_t, std::vector<node_t>>::iterator iter = visible_ids.begin(); iter != visible_ids.end(); ++iter)
            {
                model_t model_id = iter->first;

                for(node_t node_id : iter->second)
                {
                    visibility_grid_->set_cell_visibility(current_grid_index_, model_id, node_id, true);
                }
            }
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

        
        direction_counter_++;
        if(direction_counter_ == 6)
        {
            direction_counter_ = 0;

            // Measure time passed during last visibility test from given point and check whether there is time left.
            std::chrono::duration<double> time_passed_during_last_test = std::chrono::system_clock::now() - current_test_start_time_;
            current_test_start_time_ = std::chrono::system_clock::now();

            remaining_duration_visibility_test_in_seconds_ -= time_passed_during_last_test.count();

            if(remaining_duration_visibility_test_in_seconds_ < 0)
            {
                signal_shutdown = true;
            }
            else
            {
                // Re-roll new view cell.
                current_grid_index_ = view_cell_distribution_(view_cell_rng_);
                const view_cell* new_current_cell = visibility_grid_->get_cell_at_index(current_grid_index_);

                // Re-roll a position within the newly selected view cell.
                std::uniform_real_distribution<double> position_x_distribution(new_current_cell->get_position_center().x - new_current_cell->get_size().x * 0.5,
                                                                                new_current_cell->get_position_center().x + new_current_cell->get_size().x * 0.5);
                double x = position_x_distribution(view_cell_rng_);

                std::uniform_real_distribution<double> position_y_distribution(new_current_cell->get_position_center().y - new_current_cell->get_size().y * 0.5,
                                                                                new_current_cell->get_position_center().y + new_current_cell->get_size().y * 0.5);
                double y = position_y_distribution(view_cell_rng_);
                
                std::uniform_real_distribution<double> position_z_distribution(new_current_cell->get_position_center().z - new_current_cell->get_size().z * 0.5,
                                                                                new_current_cell->get_position_center().z + new_current_cell->get_size().z * 0.5);
                double z = position_z_distribution(view_cell_rng_);

                current_position_in_current_view_cell_ = scm::math::vec3d(x, y, z);

                // Update sample information.
                num_samples_per_view_cell_[current_grid_index_]++;
            }
        }

        // Calculate current rendering state so user gets visual feedback on the preprocessing progress.
        size_t current_days = (size_t)remaining_duration_visibility_test_in_seconds_ / (60 * 60 * 24);
        size_t current_hours = ((size_t)remaining_duration_visibility_test_in_seconds_ / (60 * 60)) % 24;
        size_t current_minutes = ((size_t)remaining_duration_visibility_test_in_seconds_ / 60) % 60;
        size_t current_seconds = (size_t)remaining_duration_visibility_test_in_seconds_ % 60;
        
        std::cout << "\rremaining visibility test time: [" << current_days << "d " << current_hours << "h " << current_minutes << "m " << current_seconds << "s]          " << std::flush;

        if(remaining_duration_visibility_test_in_seconds_ <= 0.0f)
        {
            std::cout << std::endl;
        }
    }

    // Once the visibility test is complete ...
    if(signal_shutdown)
    {
        // ... calculate which nodes are inside the view cells based on the average depth of the nodes inside the cuts during rendering.
        std::cout << "start check for nodes inside grid cells..." << std::endl;
        check_for_nodes_within_cells(total_depth_rendered_nodes_, total_num_rendered_nodes_);
        std::cout << "node check finished" << std::endl;

        // ... set visibility of LOD-trees based on rendered nodes.
        std::cout << "start visibility propagation..." << std::endl;
        emit_node_visibility(visibility_grid_);
        std::cout << "visibility propagation finished" << std::endl;

        // Write info collected on sampling to file.
        std::string sample_info_file_path = pvs_file_path_;
        sample_info_file_path.resize(sample_info_file_path.size() - 4);
        sample_info_file_path += "_sampling.txt";

        std::ofstream file_out;
        file_out.open(sample_info_file_path, std::ios::out);

        size_t total_samples_taken = 0;
        for(size_t index = 0; index < num_samples_per_view_cell_.size(); ++index)
        {
            file_out << "view cell " << index << ": " << num_samples_per_view_cell_[index] << std::endl;
            total_samples_taken += num_samples_per_view_cell_[index];
        }

        file_out << "\ntotal samples: " << total_samples_taken << std::endl;
        
        float average_samples_per_cell = (float)total_samples_taken / (float)num_samples_per_view_cell_.size();
        file_out << "average samples per view cell: " << average_samples_per_cell << std::endl;

        float total_derivation = 0;
        size_t smallest_sample_count = 0;
        size_t largest_sample_count = 0;

        for(size_t index = 0; index < num_samples_per_view_cell_.size(); ++index)
        {
            total_derivation += std::abs(num_samples_per_view_cell_[index] - average_samples_per_cell);

            if(index == 0)
            {
                smallest_sample_count = num_samples_per_view_cell_[index];
                largest_sample_count = num_samples_per_view_cell_[index];
            }
            else
            {
                smallest_sample_count = std::min(smallest_sample_count, num_samples_per_view_cell_[index]);
                largest_sample_count = std::max(largest_sample_count, num_samples_per_view_cell_[index]);
            }
        }

        float average_derivation = total_derivation / (float)num_samples_per_view_cell_.size();
        file_out << "\naverage sample derivation: " << average_derivation << std::endl;
        file_out << "smallest number of samples: " << smallest_sample_count << std::endl;
        file_out << "largest number of samples: " << largest_sample_count << std::endl;

        double total_duration_in_minutes = duration_visibility_test_in_seconds_ / 60.0;
        float samples_per_minute = (float)total_samples_taken / (float)total_duration_in_minutes;
        file_out << "\nsamples per minute: " << samples_per_minute << std::endl;

        file_out.close();
    }

    return signal_shutdown;
}

void management_simple_randomized::
check_for_nodes_within_cells(const std::vector<std::vector<size_t>>& total_depths, const std::vector<std::vector<size_t>>& total_nums)
{
    lamure::ren::model_database* database = lamure::ren::model_database::get_instance();

    for(model_t model_index = 0; model_index < database->num_models(); ++model_index)
    {
        for(size_t cell_index = 0; cell_index < visibility_grid_->get_cell_count(); ++cell_index)
        {
            // Create bounding box of view cell.
            const view_cell* current_cell = visibility_grid_->get_cell_at_index(cell_index);
                
            vec3r min_vertex(current_cell->get_position_center() - (current_cell->get_size() * 0.5f));
            vec3r max_vertex(current_cell->get_position_center() + (current_cell->get_size() * 0.5f));
            bounding_box cell_bounds(min_vertex, max_vertex);

            // We can get the first and last index of the nodes on a certain depth inside the bvh.
            unsigned int average_depth = database->get_model(model_index)->get_bvh()->get_depth();
            if(total_num_rendered_nodes_[model_index][cell_index] != 0)
            {
                average_depth = total_depth_rendered_nodes_[model_index][cell_index] / total_num_rendered_nodes_[model_index][cell_index];
            }

            node_t start_index = database->get_model(model_index)->get_bvh()->get_first_node_id_of_depth(average_depth);
            node_t end_index = start_index + database->get_model(model_index)->get_bvh()->get_length_of_depth(average_depth);

            for(node_t node_index = start_index; node_index < end_index; ++node_index)
            {
                // Create bounding box of node.
                scm::gl::boxf node_bounding_box = database->get_model(model_index)->get_bvh()->get_bounding_boxes()[node_index];
                vec3r min_vertex = vec3r(node_bounding_box.min_vertex()) + database->get_model(model_index)->get_bvh()->get_translation();
                vec3r max_vertex = vec3r(node_bounding_box.max_vertex()) + database->get_model(model_index)->get_bvh()->get_translation();
                bounding_box node_bounds(min_vertex, max_vertex);

                // check if the bounding boxes collide.
                if(cell_bounds.intersects(node_bounds))
                {
                    visibility_grid_->set_cell_visibility(cell_index, model_index, node_index, true);
                }
            }
        }
    }
}

void management_simple_randomized::
emit_node_visibility(grid* visibility_grid)
{
    float steps_finished = 0.0f;
    float total_steps = visibility_grid_->get_cell_count();

    // Advance node visibility downwards and upwards in the LOD-hierarchy.
    // Since only a single LOD-level was rendered in the visibility test, this is necessary to produce a complete PVS.
    #pragma omp parallel for
    for(size_t cell_index = 0; cell_index < visibility_grid->get_cell_count(); ++cell_index)
    {
        const view_cell* current_cell = visibility_grid->get_cell_at_index(cell_index);
        std::map<model_t, std::vector<node_t>> visible_indices = current_cell->get_visible_indices();

        for(std::map<model_t, std::vector<node_t>>::const_iterator map_iter = visible_indices.begin(); map_iter != visible_indices.end(); ++map_iter)
        {
            for(node_t node_index = 0; node_index < map_iter->second.size(); ++node_index)
            {
                node_t visible_node_id = map_iter->second.at(node_index);

                // Communicate visibility to children and parents nodes of visible nodes.
                set_node_children_visible(cell_index, current_cell, map_iter->first, visible_node_id);
                set_node_parents_visible(cell_index, current_cell, map_iter->first, visible_node_id);
            }       
        }

        #pragma omp critical
        {
            // Calculate current node propagation state so user gets visual feedback on the preprocessing progress.
            steps_finished++;
            float current_percentage_done = (steps_finished / total_steps) * 100.0f;
            std::cout << "\rvisibility propagation in progress [" << current_percentage_done << "]       " << std::flush;
        }
    }

    std::cout << std::endl;
}

void management_simple_randomized::
set_node_parents_visible(const size_t& cell_id, const view_cell* cell, const model_t& model_id, const node_t& node_id)
{
    // Set parents of a visible node visible, too.
    // Necessary since only a single LOD-level is rendered during the visibility test.
    lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
    node_t parent_id = database->get_model(model_id)->get_bvh()->get_parent_id(node_id);
    
    if(parent_id != lamure::invalid_node_t && !cell->get_visibility(model_id, parent_id))
    {
        visibility_grid_->set_cell_visibility(cell_id, model_id, parent_id, true);
        set_node_parents_visible(cell_id, cell, model_id, parent_id);
    }
}

void management_simple_randomized::
set_node_children_visible(const size_t& cell_id, const view_cell* cell, const model_t& model_id, const node_t& node_id)
{
    // Set children of a visible node visible, too.
    lamure::ren::model_database* database = lamure::ren::model_database::get_instance();
    uint32_t fan_factor = database->get_model(model_id)->get_bvh()->get_fan_factor();

    for(uint32_t child_index = 0; child_index < fan_factor; ++child_index)
    {
        node_t child_id = database->get_model(model_id)->get_bvh()->get_child_id(node_id, child_index);
        if(child_id < database->get_model(model_id)->get_bvh()->get_num_nodes() && !cell->get_visibility(model_id, child_id))
        {
            visibility_grid_->set_cell_visibility(cell_id, model_id, child_id, true);
            set_node_children_visible(cell_id, cell, model_id, child_id);
        }
    }
}

void management_simple_randomized::
update_trackball(int x, int y)
{
}

void management_simple_randomized::
RegisterMousePresses(int button, int state, int x, int y)
{
}

void management_simple_randomized::
dispatchKeyboardInput(unsigned char key)
{
}

void management_simple_randomized::
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

void management_simple_randomized::
Toggledispatching()
{
    dispatch_ = ! dispatch_;
}

void management_simple_randomized::
DecreaseErrorThreshold()
{
    error_threshold_ -= 0.1f;
    if (error_threshold_ < LAMURE_MIN_THRESHOLD)
    {
        error_threshold_ = LAMURE_MIN_THRESHOLD;
    }
}

void management_simple_randomized::
IncreaseErrorThreshold()
{
    error_threshold_ += 0.1f;
    if (error_threshold_ > LAMURE_MAX_THRESHOLD)
    {
        error_threshold_ = LAMURE_MAX_THRESHOLD;
    }
}

void management_simple_randomized::
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

void management_simple_randomized::
set_pvs_file_path(const std::string& file_path)
{
    pvs_file_path_ = file_path;
}

void management_simple_randomized::
set_duration_visibility_test(const double& duration)
{
    duration_visibility_test_in_seconds_ = duration;
}

}
}
