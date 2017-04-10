// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "utils.h"
#include "management.h"
#include <set>
#include <ctime>
#include <algorithm>
#include <fstream>
#include <lamure/lod/bvh.h>
#include <lamure/assert.h>
#include <lamure/math/std_math.h>
#include <lamure/math/gl_math.h>

management::
management(std::vector<std::string> const& model_filenames,
    std::vector<lamure::mat4r_t> const& model_transformations,
    std::set<lamure::model_t> const& visible_set,
    std::set<lamure::model_t> const& invisible_set,
    snapshot_session_descriptor& snap_descriptor)
    :   num_taken_screenshots_(0),
        allow_user_input_(snap_descriptor.recorded_view_vector_.size() == 0),
        screenshot_session_started_(false),
        camera_recording_enabled_(false),
        //current_session_filename_(session_filename),
        current_session_file_path_(""),
        num_recorded_camera_positions_(0),
        renderer_(nullptr),
        model_filenames_(model_filenames),
        model_transformations_(model_transformations),
        //recorded_view_vector_(recorded_view_vector),
        measurement_session_descriptor_(snap_descriptor),
#ifdef LAMURE_RENDERING_USE_SPLIT_SCREEN
        active_camera_left_(nullptr),
        active_camera_right_(nullptr),
        control_left_(true),
#endif
        test_send_rendered_(true),
        active_camera_(nullptr),
        mouse_state_(),
        num_models_(0),
        num_cameras_(1),
        fast_travel_(false),
        dispatch_(true),
        trigger_one_update_(false),
        reset_matrix_(lamure::math::mat4d_t::identity()),
        reset_diameter_(90.f),
        detail_translation_(lamure::math::vec3d_t(0.0)),
        detail_angle_(0.f),
        error_threshold_(LAMURE_DEFAULT_THRESHOLD),
        near_plane_(0.001f),
        far_plane_(100.f),
        importance_(1.f)

{

    lamure::lod::model_database* database = lamure::lod::model_database::get_instance();

#ifdef LAMURE_RENDERING_ENABLE_LAZY_MODELS_TEST
    ASSERT(model_filenames_.size() > 0);
    lamure::model_t model_id = database->add_model(model_filenames_[0], std::to_string(num_models_));
    ++num_models_;
#else
    for (const auto& filename : model_filenames_)
    {
        lamure::model_t model_id = database->add_model(filename, std::to_string(num_models_));
        ++num_models_;
    }
#endif

    {

        double scene_diameter = far_plane_;
        for (lamure::model_t model_id = 0; model_id < database->num_models(); ++model_id) {
            const auto& bb = database->get_model(model_id)->get_bvh()->get_bounding_boxes()[0];
            scene_diameter = lamure::math::max(lamure::math::length(bb.max()-bb.min()), scene_diameter);
            model_transformations_[model_id] = model_transformations_[model_id] * lamure::math::make_translation(database->get_model(model_id)->get_bvh()->get_translation());
        }
        far_plane_ = 2.0f * scene_diameter;

        auto root_bb = database->get_model(0)->get_bvh()->get_bounding_boxes()[0];
        lamure::math::vec3d_t center = model_transformations_[0] * ((root_bb.min() + root_bb.max()) / 2.0);
        reset_matrix_ = lamure::math::make_look_at_matrix(center+lamure::math::vec3d_t(0.0, 0.1,-0.01), center, lamure::math::vec3d_t(0.0, 1.0,0.0));
        reset_diameter_ = lamure::math::length(root_bb.max()-root_bb.min());


        std::cout << "model center : " << center.x_ << " " << center.y_ << " " << center.z_ << std::endl;
        std::cout << "model size : " << reset_diameter_ << std::endl;

        for (lamure::view_t cam_id = 0; cam_id < num_cameras_; ++cam_id)
        {
            cameras_.push_back(new lamure::lod::camera(cam_id, reset_matrix_, reset_diameter_, false, false));
        }
    }

#ifdef LAMURE_RENDERING_USE_SPLIT_SCREEN
    active_camera_left_ = cameras_[0];
    active_camera_right_ = cameras_[0];
#endif
    active_camera_ = cameras_[0];

#ifdef LAMURE_RENDERING_USE_SPLIT_SCREEN
    renderer_ = new SplitScreenRenderer(model_transformations_);
#endif

#ifndef LAMURE_RENDERING_USE_SPLIT_SCREEN
    renderer_ = new Renderer(model_transformations_, visible_set, invisible_set);
#endif

    PrintInfo();
}

management::
~management()
{
    for (auto& cam : cameras_)
    {
        if (cam != nullptr)
        {
            delete cam;
            cam = nullptr;
        }
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
    lamure::lod::model_database* database = lamure::lod::model_database::get_instance();
    lamure::lod::controller* controller = lamure::lod::controller::get_instance();
    lamure::lod::cut_database* cuts = lamure::lod::cut_database::get_instance();

    bool signal_shutdown = false;

#if 0
    for (unsigned int model_id = 0; model_id < database->num_models(); ++model_id) {
       model_transformations_[model_id] = model_transformations_[model_id] * lamure::math::make_translation(28.f, -389.f, -58.f);
       renderer_->send_model_transform(model_id, model_transformations_[model_id]);
    }

#endif

    controller->reset_system();

    lamure::context_t context_id = controller->deduce_context_id(0);
    
    controller->dispatch(context_id/*, renderer_->device()*/);


#ifdef LAMURE_RENDERING_USE_SPLIT_SCREEN
    lamure::view_t view_id_left = controller->deduce_view_id(context_id, active_camera_left_->view_id());
    lamure::view_t view_id_right = controller->deduce_view_id(context_id, active_camera_right_->view_id());
#else

    lamure::view_t view_id = controller->deduce_view_id(context_id, active_camera_->view_id());

#endif    
    

#ifdef LAMURE_RENDERING_USE_SPLIT_SCREEN
    renderer_->render(context_id, *active_camera_left_, view_id_left, 0, controller->get_context_memory(context_id, lamure::lod::bvh::primitive_type::POINTCLOUD),controller->get_context_buffer(context_id), num_recorded_camera_positions_);
    renderer_->render(context_id, *active_camera_right_, view_id_right, 1, controller->get_context_memory(context_id, lamure::lod::bvh::primitive_type::POINTCLOUD), controller->get_context_buffer(context_id), num_recorded_camera_positions_);
#else
    renderer_->set_radius_scale(importance_);
    renderer_->render(context_id, *active_camera_, view_id, 
      controller->get_context_memory(context_id, lamure::lod::bvh::primitive_type::POINTCLOUD), 
      controller->get_context_buffer(context_id), num_recorded_camera_positions_);
#endif


    std::string status_string("");

    if(camera_recording_enabled_) {
        status_string += "Session recording (#"+std::to_string(current_session_number_) +") : ON\n";
    } else {
        status_string += "Session recording: OFF\n";
    }

    if (! allow_user_input_) {


        status_string += std::to_string(measurement_session_descriptor_.recorded_view_vector_.size()+1) + " views left to write.\n";

        if ( !screenshot_session_started_ ) {
            
        }

        size_t ms_since_update = controller->ms_since_last_node_upload();

        if ( ms_since_update > 3000) {
            if ( screenshot_session_started_ )
                
                if(measurement_session_descriptor_.get_num_taken_screenshots() ) {
                    auto const& resolution = measurement_session_descriptor_.snapshot_resolution_;
                    renderer_->take_screenshot("../quality_measurement/session_screenshots/" + measurement_session_descriptor_.session_filename_, 
                                                measurement_session_descriptor_.get_screenshot_name() );
                }

                measurement_session_descriptor_.increment_screenshot_counter();

            if(! measurement_session_descriptor_.recorded_view_vector_.empty() ) {
                if (! screenshot_session_started_ ) {
                    screenshot_session_started_ = true;
                }
                active_camera_->set_view_matrix(measurement_session_descriptor_.recorded_view_vector_.back());
                controller->reset_ms_since_last_node_upload();
                measurement_session_descriptor_.recorded_view_vector_.pop_back();
            } else {
                // leave the main loop
                signal_shutdown = true;
            }

        } else {
            status_string += std::to_string( ((3000 - ms_since_update) / 100) * 100 ) + " ms until next buffer snapshot.\n";
        }
    }

    renderer_->display_status(status_string);

    if (dispatch_ || trigger_one_update_)
    {
        if (trigger_one_update_)
        {
            trigger_one_update_ = false;
        }

        for (lamure::model_t model_id = 0; model_id < num_models_; ++model_id)
        {
            lamure::model_t m_id = controller->deduce_model_id(std::to_string(model_id));

            cuts->send_transform(context_id, m_id, model_transformations_[m_id]);
            cuts->send_threshold(context_id, m_id, error_threshold_ / importance_);
            
            //if (visible_set_.find(model_id) != visible_set_.end())
            if (!test_send_rendered_) {
               if (model_id > num_models_/2) {
                  cuts->send_rendered(context_id, m_id);
               }
            }
            else {
               cuts->send_rendered(context_id, m_id);
            }

            database->get_model(m_id)->set_transform(model_transformations_[m_id]);
        }

        for (auto& cam : cameras_)
        {
            lamure::view_t cam_id = controller->deduce_view_id(context_id, cam->view_id());
            cuts->send_camera(context_id, cam_id, *cam);

            std::vector<lamure::vec3d_t> corner_values = cam->get_frustum_corners();
            double top_minus_bottom = lamure::math::length((corner_values[2]) - (corner_values[0]));
            float height_divided_by_top_minus_bottom = lamure::lod::policy::get_instance()->window_height() / top_minus_bottom;

            cuts->send_height_divided_by_top_minus_bottom(context_id, cam_id, height_divided_by_top_minus_bottom);
        }

        //controller->dispatch(context_id, renderer_->device());


    }

#ifdef LAMURE_CUT_UPDATE_ENABLE_MEASURE_SYSTEM_PERFORMANCE
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

#endif

    return signal_shutdown;
}


void management::
update_trackball(int x, int y)
{
    active_camera_->update_trackball(x,y, width_, height_, mouse_state_);
}



void management::
RegisterMousePresses(int button, int state, int x, int y)
{
    if(! allow_user_input_) {
        return;
    }

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

}


void management::
dispatchKeyboardInput(unsigned char key)
{

    if(! allow_user_input_) {
        return;
    }

    bool override_center_of_rotation = false;

    switch (key)
    {
    case '+':
        importance_ += 0.1f;
        importance_ = std::min(importance_, 1.0f);
        std::cout << "importance: " << importance_ << std::endl;
        break;

    case '-':
        importance_ -= 0.1f;
        importance_ = std::max(0.1f, importance_);
        std::cout << "importance: " << importance_ << std::endl;
        break;

    case 'y':
        test_send_rendered_ = !test_send_rendered_;
        std::cout << "send rendered: " << test_send_rendered_ << std::endl;
        break;
    case 'w':
        renderer_->toggle_bounding_box_rendering();
        break;
    case 'U':
        renderer_->change_point_size(1.0f);
        break;
    case 'u':
        renderer_->change_point_size(0.1f);
        break;
    case 'J':
        renderer_->change_point_size(-1.0f);
        break;
    case 'j':
        renderer_->change_point_size(-0.1f);
        break;
    case 't':
#ifndef LAMURE_RENDERING_USE_SPLIT_SCREEN
        renderer_->toggle_visible_set();
#endif
        break;

#ifdef LAMURE_RENDERING_USE_SPLIT_SCREEN
    case '1':

        control_left_ = !control_left_;
        if (control_left_)
            active_camera_ = active_camera_left_;
        else
            active_camera_ = active_camera_right_;
        break;

    case '2':
        control_left_ = !control_left_;
        if (control_left_)
            active_camera_ = active_camera_left_;
        else
            active_camera_ = active_camera_right_;
#else
    case '1':
        renderer_->switch_render_mode(RenderMode::HQ_ONE_PASS);
        break;

    case '2':
        renderer_->switch_render_mode(RenderMode::HQ_TWO_PASS);
        break;

    case '3':
        renderer_->switch_render_mode(RenderMode::LQ_ONE_PASS);
        break;
#endif

    case ' ':
#ifdef LAMURE_RENDERING_ENABLE_MULTI_VIEW_TEST
#ifdef LAMURE_RENDERING_USE_SPLIT_SCREEN
        {
            if (control_left_)
            {
                lamure::view_t current_camera_id = active_camera_left_->view_id();
                active_camera_left_ = cameras_[(++current_camera_id) % num_cameras_];
                active_camera_ = active_camera_left_;
            }
            else
            {
                lamure::view_t current_camera_id = active_camera_right_->view_id();
                active_camera_right_ = cameras_[(++current_camera_id) % num_cameras_];
                active_camera_ = active_camera_right_;
            }

            renderer_->toggle_camera_info(active_camera_left_->view_id(), active_camera_right_->view_id());
        }
#else
        {
            lamure::view_t current_camera_id = active_camera_->view_id();
            active_camera_ = cameras_[(++current_camera_id) % num_cameras_];
            renderer_->toggle_camera_info(active_camera_->view_id());
        }
#endif
#endif
        break;

    case 'd':
        this->Toggledispatching();
        renderer_->toggle_cut_update_info();
        break;

    case 'z':
        {
        lamure::lod::ooc_cache* ooc_cache = lamure::lod::ooc_cache::get_instance();
        ooc_cache->begin_measure();
        }
        break;

    case 'Z':
        {
        lamure::lod::ooc_cache* ooc_cache = lamure::lod::ooc_cache::get_instance();
        ooc_cache->end_measure();
        }
        break;

    case 'e':
        trigger_one_update_ = true;
        break;

    case 'x':
        {
#ifdef LAMURE_RENDERING_ENABLE_MULTI_VIEW_TEST
        cameras_.push_back(new lamure::lod::camera(num_cameras_, reset_matrix_, reset_diameter_, fast_travel_));
        int w = width_;
        int h = height_;
#ifdef LAMURE_RENDERING_USE_SPLIT_SCREEN
        w/=2;
#endif

        cameras_.back()->set_projection_matrix(30.0f, float(w)/float(h),  near_plane_, far_plane_);

        ++num_cameras_;
#endif
        }
        break;


    case 'f':
        std::cout<<"fast travel: ";
        if(fast_travel_)
        {
            for (auto& cam : cameras_)
            {
                cam->set_dolly_sens_(0.5f);
            }
            std::cout<<"OFF\n\n";
        }
        else
        {
            for (auto& cam : cameras_)
            {
                cam->set_dolly_sens_(20.5f);
            }
            std::cout<<"ON\n\n";
        }

        fast_travel_ = ! fast_travel_;

        break;

    case 'r':
    case 'R':
        toggle_camera_session();
        break;

    case 'a':
        record_next_camera_position();
        break;

    case '0':
        active_camera_->set_trackball_matrix(reset_matrix_);
        break;

    case '9':
        renderer_->toggle_display_info();
        break;

    case 'k':
        DecreaseErrorThreshold();
        std::cout << "error threshold: " << error_threshold_ << std::endl;
        break;
    case 'i':
        IncreaseErrorThreshold();
        std::cout << "error threshold: " << error_threshold_ << std::endl;
        break;
    }
}

void management::
dispatchResize(int w, int h)
{
    width_ = w;
    height_ = h;

#ifdef LAMURE_RENDERING_USE_SPLIT_SCREEN
    w/=2;
#endif

    // if snapshots are taken, use the user specified resolution
    if( measurement_session_descriptor_.snapshot_session_enabled_ ) {
        renderer_->reset_viewport(measurement_session_descriptor_.snapshot_resolution_[0],
                                  measurement_session_descriptor_.snapshot_resolution_[1]);
    } else { // otherwise react on window resizing 
        renderer_->reset_viewport(w,h);
    }
    lamure::lod::policy* policy = lamure::lod::policy::get_instance();
    policy->set_window_width(w);
    policy->set_window_height(h);


    for (auto& cam : cameras_)
    {
        if(measurement_session_descriptor_.snapshot_session_enabled_ ) {
            cam->set_projection_matrix(30.0f, float(measurement_session_descriptor_.snapshot_resolution_[0])/float(measurement_session_descriptor_.snapshot_resolution_[1]),  near_plane_, far_plane_);
        }
        else {
            cam->set_projection_matrix(30.0f, float(w)/float(h),  near_plane_, far_plane_);
        }
    }
}

void management::
Toggledispatching()
{
    dispatch_ = ! dispatch_;
};


void management::
DecreaseErrorThreshold() {
    error_threshold_ -= 0.1f;
    if (error_threshold_ < LAMURE_MIN_THRESHOLD)
        error_threshold_ = LAMURE_MIN_THRESHOLD;

}

void management::
IncreaseErrorThreshold() {
    error_threshold_ += 0.1f;
    if (error_threshold_ > LAMURE_MAX_THRESHOLD)
        error_threshold_ = LAMURE_MAX_THRESHOLD;

}

void management::
PrintInfo()
{
    std::cout<<"\n"<<
               "Controls: w - enable/disable bounding box rendering\n"<<
               "\n"<<
               "          U/u - increase point size by 1.0/0.1\n"<<
               "          J/j - decrease point size by 1.0/0.1\n"<<
               "\n"<<
               "          o - switch to circle/ellipse rendering\n"<<
               "          c - toggle normal clamping\n"<<
               "\n"<<
               "          A/a - increase clamping ratio by 0.1/0.01f\n"<<
               "          S/s - decrease clamping ratio by 0.1/0.01f\n"<<
               "\n"<<
               "          d - toggle dispatching\n"<<
               "          e - trigger 1 dispatch, if dispatch is frozen\n"<<
               "          f - toggle fast travel\n"<<
               "          . - toggle fullscreen\n"<<
               "\n"<< 
               "          i - increase error threshold\n" <<
               "          k - decrease error threshold\n" <<
               "\n"<< 
               "          + (NUMPAD) - increase importance\n" <<
               "          - (NUMPAD) - decrease importance\n" <<


#ifdef LAMURE_RENDERING_ENABLE_MULTI_VIEW_TEST
               "\n"<<
               "          x - add camera\n"<<
               "          Space - switch to next camera\n" <<
#ifdef LAMURE_RENDERING_USE_SPLIT_SCREEN
               "\n"<<
               "          1 - control left screen"<<
               "\n"<<
               "          2 - control right screen"<<
#endif
#endif

               "\n";


}

void management::
toggle_camera_session() {
    camera_recording_enabled_ = !camera_recording_enabled_;
}

void management::
record_next_camera_position() {
    if (camera_recording_enabled_) {
        create_quality_measurement_resources();
    }
}

void management::
create_quality_measurement_resources() {
#if 0
    std::string base_quality_measurement_path = "../quality_measurement/";
    std::string session_file_prefix = "session_";

    if(! boost::filesystem::exists(base_quality_measurement_path)) {
        std::cout<<"Creating Folder.\n\n";
        boost::filesystem::create_directories(base_quality_measurement_path);
    }

    if( current_session_file_path_.empty() ) {

        boost::filesystem::directory_iterator begin(base_quality_measurement_path), end;
        
        int num_existing_sessions = std::count_if(begin, end,
            [](const boost::filesystem::directory_entry & d) {
                return !boost::filesystem::is_directory(d.path());
            });

        current_session_number_ = num_existing_sessions+1;

        current_session_file_path_ = base_quality_measurement_path+session_file_prefix+std::to_string(current_session_number_)+".csn";
        

    }

    std::ofstream camera_session_file(current_session_file_path_, std::ios_base::out | std::ios_base::app);
    active_camera_->write_view_matrix(camera_session_file);
    camera_session_file.close();
#endif
}
