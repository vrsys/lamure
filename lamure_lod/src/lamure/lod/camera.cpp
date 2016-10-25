// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/lod/camera.h>

namespace lamure {
namespace lod {

std::mutex  camera::transform_update_mutex_;

camera::
camera(const view_t view_id,
       float64_t near_plane,
       mat4r_t const& view,
       mat4r_t const& proj)
      : view_id_(view_id),
      view_matrix_(view),
      projection_matrix_(proj),
      near_plane_value_(near_plane),
      far_plane_value_(1000.0f),
      trackball_init_x_(0.0),
      trackball_init_y_(0.0),
      dolly_sens_(0.5f),
      is_in_touch_screen_mode_(0),
      sum_trans_x_(0), sum_trans_y_(0), sum_trans_z_(0),
      sum_rot_x_(0), sum_rot_y_(0), sum_rot_z_(0),
      cam_state_(CAM_STATE_GUA)
{
   frustum_ = lamure::util::frustum_t(proj * view);

}

camera::
camera(const view_t view_id,
       mat4r_t const& init_tb_mat,
       float64_t distance,
       bool fast_travel,
       bool touch_screen_mode)
    : view_id_(view_id),
      near_plane_value_(0),
      far_plane_value_(0),
      trackball_init_x_(0.0),
      trackball_init_y_(0.0),
      dolly_sens_(fast_travel ? 20.0 : 0.5),
      is_in_touch_screen_mode_(touch_screen_mode),
      sum_trans_x_(0), sum_trans_y_(0), sum_trans_z_(0),
      sum_rot_x_(0), sum_rot_y_(0), sum_rot_z_(0),
      cam_state_(CAM_STATE_LAMURE)
{
       // set_projection_matrix(30.0f, float64_t(800)/float64_t(600), 0.01f, 100.0f);
       // lamure::math::perspective_matrix(projection_matrix_, 60.f, float64_t(800)/float64_t(600), 0.1f, 100.0f);
        //frustum_ = lamure::math::frustum_t(projection_matrix_);
      //  lamure::math::perspective_matrix(projection_matrix_, 60.f, float64_t(800)/float64_t(600), 0.1f, 100.0f);


    trackball_.set_transform(mat4r_t(init_tb_mat));
    trackball_.dolly(distance);

}

camera::
~camera()
{

}


void camera::set_trackball_center_of_rotation(const vec3r_t& cor) 
{
    
   trackball_.set_dolly(0.f);
   mat4r_t cm = lamure::math::inverse(mat4r_t(trackball_.transform()));
   vec3r_t pos = vec3r_t(cm[12],cm[13],cm[14]); 

   if (lamure::math::length(pos - cor) < 0.001f) {
      return;
   }

   vec3r_t up = vec3r_t(cm[4], cm[5], cm[6]);
        
   mat4r_t look_at = lamure::math::make_look_at_matrix(cor + vec3r_t(0.f, 0.f, 0.001f), cor , up);

   trackball_.set_transform(mat4r_t::identity());
   trackball_.set_transform(mat4r_t(look_at));
   trackball_.dolly(lamure::math::length(pos-(cor + vec3r_t(0.f, 0.f, 0.001f)))); 

}


void camera::event_callback(uint16_t code, float64_t value)
{
  std::lock_guard<std::mutex> lock(transform_update_mutex_);

  float64_t const transV = 3.0f;
  float64_t const rotV = 5.0f;
  float64_t const rotVz = 15.0f;


  if (std::abs(value) < 0.0)
    value = 0;

  if (is_in_touch_screen_mode_ == true)
  {
    switch (code)
    {
    case 1:
      code = 2;
      break;
    case 2:
      value = -value;
      code = 1;
      break;
    case 4:

      code = 5;
      break;
    case 5:
      value = -value;
      code = 4;
      break;
    }
  }

  if (code == 0)
  {
    sum_trans_x_ = remap_value(-value, -500, 500, -transV, transV);
  }
  if (code == 2)
  {
    sum_trans_y_ = remap_value(value, -500, 500, -transV, transV);
  }
  if (code == 1)
  {
    sum_trans_z_ = remap_value(-value, -500, 500, -transV, transV);
  }
  if (code == 3)
  {
    sum_rot_x_ = remap_value(-value, -500, 500, -rotV, rotV); //0
  }
  if (code == 5)
  {
    sum_rot_y_ = remap_value(value, -500, 500, -rotV, rotV); //0
  }
  if (code == 4)
  {
    sum_rot_z_ = remap_value(-value, -500, 500, -rotVz, rotVz);
  }

}

lamure::util::frustum_t::classification_result_t const camera::
cull_against_frustum(lamure::util::frustum_t const& frustum, lamure::math::bounding_box_t const & b) const
{
    return frustum.classify(b);
}

lamure::util::frustum_t const camera::get_frustum_by_model(mat4r_t const& model) const
{
    switch (cam_state_) {
        case CAM_STATE_LAMURE:
            return lamure::util::frustum_t(this->projection_matrix_ * mat4r_t(trackball_.transform()) * model);
            break;


        case CAM_STATE_GUA:
            return lamure::util::frustum_t(this->projection_matrix_ * view_matrix_ * model);
            break;

        default: break;
    }

    return lamure::util::frustum_t();
}


void camera::
set_projection_matrix(float64_t opening_angle, float64_t aspect_ratio, float64_t near, float64_t far)
{
    lamure::math::perspective_matrix(projection_matrix_, opening_angle, aspect_ratio, near, far);

    near_plane_value_   = near;
    far_plane_value_    = far;

    frustum_ = lamure::util::frustum_t(this->projection_matrix_ * mat4r_t(trackball_.transform()));
}

void camera::
set_view_matrix(mat4r_t const& in_view ) {
  switch (cam_state_) {
    case CAM_STATE_LAMURE:
      trackball_.set_transform(in_view);
      break;

    case CAM_STATE_GUA:
      view_matrix_ = in_view;
      break;

      default: break;
    }
}

void camera::
update_trackball_mouse_pos(float64_t x, float64_t y)
{
    trackball_init_x_ = x;
    trackball_init_y_ = y;
}

void camera::
update_trackball(int32_t x, int32_t y, int32_t window_width, int32_t window_height, mouse_state const& mouse_state)
{

    float64_t nx = 2.0 * float64_t(x - (window_width/2))/float64_t(window_width);
    float64_t ny = 2.0 * float64_t(window_height - y - (window_height/2))/float64_t(window_height);

    if (mouse_state.lb_down_) {
        trackball_.rotate(trackball_init_x_, trackball_init_y_, nx, ny);
    }
    if (mouse_state.rb_down_) {
        trackball_.dolly(dolly_sens_*0.5 * (ny - trackball_init_y_));
    }
    if (mouse_state.mb_down_) {
        float64_t f = dolly_sens_ < 1.0 ? 0.02 : 0.3;
        trackball_.translate(f*(nx - trackball_init_x_), f*(ny - trackball_init_y_));
    }

    trackball_init_y_ = ny;
    trackball_init_x_ = nx;


}

void camera::
write_view_matrix(std::ofstream& matrix_stream)
{
    mat4r_t t_mat = trackball_.transform();
    matrix_stream << t_mat[ 0]<<" "<<t_mat[ 1]<<" "<< t_mat[ 2]<<" "<<t_mat[ 3]<<" "
                  << t_mat[ 4]<<" "<<t_mat[ 5]<<" "<< t_mat[ 6]<<" "<<t_mat[ 7]<<" "
                  << t_mat[ 8]<<" "<<t_mat[ 9]<<" "<< t_mat[10]<<" "<<t_mat[11]<<" "
                  << t_mat[12]<<" "<<t_mat[13]<<" "<< t_mat[14]<<" "<<t_mat[15]<<"\n";


}


float64_t const camera::
transfer_values(float64_t currentValue, float64_t maxValue) const
{
    return std::pow( (std::abs(currentValue) / std::abs(maxValue) ), 4);
}

float64_t const camera::
remap_value(float64_t value, float64_t oldMin, float64_t oldMax, float64_t newMin, float64_t newMax) const
{
    float64_t intermediateValue = ((( value - oldMin) * (newMax - newMin)) / (oldMax - oldMin)) + newMin;


    return transfer_values(intermediateValue, newMax) * intermediateValue;
}

mat4r_t const camera::
get_view_matrix() const
{
    switch (cam_state_)
    {
        case CAM_STATE_LAMURE:
            return mat4r_t(trackball_.transform());
            break;

        case CAM_STATE_GUA:
            return view_matrix_;
            break;

        default: break;
    }

    return mat4r_t();
}

mat4r_t const camera::
get_high_precision_view_matrix() const {
  
  if (cam_state_ == CAM_STATE_LAMURE) {
     return trackball_.transform();
  }

  return mat4r_t(view_matrix_);

}

mat4r_t const camera::
get_projection_matrix() const
{
    return projection_matrix_;
}

std::vector<vec3r_t> camera::get_frustum_corners() const
{
  std::vector<vec4r_t> tmp(8);
  std::vector<vec3r_t> result(8);

  mat4r_t inverse_transform;

  if(CAM_STATE_LAMURE == cam_state_) {
      inverse_transform = lamure::math::inverse(mat4r_t(projection_matrix_) * trackball_.transform());
  }
  else if(CAM_STATE_GUA == cam_state_) {
      inverse_transform = mat4r_t(lamure::math::inverse(projection_matrix_ * view_matrix_));
  }

  tmp[0] = inverse_transform * vec4r_t(-1, -1, -1, 1);
  tmp[1] = inverse_transform * vec4r_t(-1, -1,  1, 1);
  tmp[2] = inverse_transform * vec4r_t(-1,  1, -1, 1);
  tmp[3] = inverse_transform * vec4r_t(-1,  1,  1, 1);
  tmp[4] = inverse_transform * vec4r_t( 1, -1, -1, 1);
  tmp[5] = inverse_transform * vec4r_t( 1, -1,  1, 1);
  tmp[6] = inverse_transform * vec4r_t( 1,  1, -1, 1);
  tmp[7] = inverse_transform * vec4r_t( 1,  1,  1, 1);

  for (int32_t i(0); i<8; ++i) {
    vec4r_t c = tmp[i]/tmp[i][3];
    result[i] = vec3r_t(c.x_, c.y_, c.z_);
  }

  return result;

}


}

}
