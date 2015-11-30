// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/ren/camera.h>


namespace lamure
{

namespace ren
{

std::mutex  Camera::transform_update_mutex_;

Camera::
Camera(const view_t view_id,
       float near_plane,
       scm::math::mat4f const& view,
       scm::math::mat4f const& proj)
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
   frustum_ = scm::gl::frustum(proj * view);

}

Camera::
Camera(const view_t view_id,
       scm::math::mat4f const& init_tb_mat,
       float distance,
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
       // SetProjectionMatrix(30.0f, float(800)/float(600), 0.01f, 100.0f);
       // scm::math::perspective_matrix(projection_matrix_, 60.f, float(800)/float(600), 0.1f, 100.0f);
        //frustum_ = scm::gl::frustum(projection_matrix_);
      //  scm::math::perspective_matrix(projection_matrix_, 60.f, float(800)/float(600), 0.1f, 100.0f);


    trackball_.set_transform(scm::math::mat4d(init_tb_mat));
    trackball_.Dolly(distance);

}

Camera::
~Camera()
{

}


void Camera::SetTrackballCenterOfRotation(const scm::math::vec3f& cor) 
{
    
   trackball_.set_dolly(0.f);
   scm::math::mat4f cm = scm::math::inverse(scm::math::mat4f(trackball_.transform()));
   scm::math::vec3f pos = scm::math::vec3f(cm[12],cm[13],cm[14]); 

   if (scm::math::length(pos - cor) < 0.001f) {
      return;
   }

   scm::math::vec3f up = scm::math::vec3f(cm[4], cm[5], cm[6]);
        
   scm::math::mat4 look_at = scm::math::make_look_at_matrix(cor + scm::math::vec3f(0.f, 0.f, 0.001f), cor , up);

   trackball_.set_transform(scm::math::mat4d::identity());
   trackball_.set_transform(scm::math::mat4d(look_at));
   trackball_.Dolly(scm::math::length(pos-(cor + scm::math::vec3f(0.f, 0.f, 0.001f)))); 

}


void Camera::event_callback(uint16_t code, float value)
{
  std::lock_guard<std::mutex> lock(transform_update_mutex_);

  float const transV = 3.0f;
  float const rotV = 5.0f;
  float const rotVz = 15.0f;


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
    sum_trans_x_ = RemapValue(-value, -500, 500, -transV, transV);
  }
  if (code == 2)
  {
    sum_trans_y_ = RemapValue(value, -500, 500, -transV, transV);
  }
  if (code == 1)
  {
    sum_trans_z_ = RemapValue(-value, -500, 500, -transV, transV);
  }
  if (code == 3)
  {
    sum_rot_x_ = RemapValue(-value, -500, 500, -rotV, rotV); //0
  }
  if (code == 5)
  {
    sum_rot_y_ = RemapValue(value, -500, 500, -rotV, rotV); //0
  }
  if (code == 4)
  {
    sum_rot_z_ = RemapValue(-value, -500, 500, -rotVz, rotVz);
  }

}

scm::gl::frustum::classification_result const Camera::
CullAgainstFrustum(scm::gl::frustum const& frustum, scm::gl::box const & b) const
{
    return frustum.classify(b);
}

scm::gl::frustum const Camera::GetFrustumByModel(scm::math::mat4 const& model) const
{
    switch (cam_state_) {
        case CAM_STATE_LAMURE:
            return scm::gl::frustum(this->projection_matrix_ * scm::math::mat4f(trackball_.transform()) * model);
            break;


        case CAM_STATE_GUA:
            return scm::gl::frustum(this->projection_matrix_ * view_matrix_ * model);
            break;

        default: break;
    }

    return scm::gl::frustum();
}


void Camera::
SetProjectionMatrix(float opening_angle, float aspect_ratio, float near, float far)
{
    scm::math::perspective_matrix(projection_matrix_, opening_angle, aspect_ratio, near, far);

    near_plane_value_   = near;
    far_plane_value_    = far;

    frustum_ = scm::gl::frustum(this->projection_matrix_ * scm::math::mat4f(trackball_.transform()));
}

void Camera::
UpdateTrackballMousePos(double x, double y)
{
    trackball_init_x_ = x;
    trackball_init_y_ = y;
}

void Camera::
UpdateTrackball(int x, int y, int window_width, int window_height, Mousestate const& mouse_state)
{

    double nx = 2.0 * double(x - (window_width/2))/double(window_width);
    double ny = 2.0 * double(window_height - y - (window_height/2))/double(window_height);

    if (mouse_state.lb_down_) {
        trackball_.Rotate(trackball_init_x_, trackball_init_y_, nx, ny);
    }
    if (mouse_state.rb_down_) {
        trackball_.Dolly(dolly_sens_*0.5 * (ny - trackball_init_y_));
    }
    if (mouse_state.mb_down_) {
        double f = dolly_sens_ < 1.0 ? 0.02 : 0.3;
        trackball_.Translate(f*(nx - trackball_init_x_), f*(ny - trackball_init_y_));
    }

    trackball_init_y_ = ny;
    trackball_init_x_ = nx;


}

void Camera::
writeViewMatrix(std::ofstream& matrix_stream)
{
    scm::math::mat4d t_mat = trackball_.transform();
    matrix_stream << t_mat[ 0]<<" "<<t_mat[ 1]<<" "<< t_mat[ 2]<<" "<<t_mat[ 3]<<" "
                  << t_mat[ 4]<<" "<<t_mat[ 5]<<" "<< t_mat[ 6]<<" "<<t_mat[ 7]<<" "
                  << t_mat[ 8]<<" "<<t_mat[ 9]<<" "<< t_mat[10]<<" "<<t_mat[11]<<" "
                  << t_mat[12]<<" "<<t_mat[13]<<" "<< t_mat[14]<<" "<<t_mat[15]<<"\n";


}


float const Camera::
TransferValues(float currentValue, float maxValue) const
{
    return std::pow( (std::abs(currentValue) / std::abs(maxValue) ), 4);
}

float const Camera::
RemapValue(float value, float oldMin, float oldMax, float newMin, float newMax) const
{
    float intermediateValue = ((( value - oldMin) * (newMax - newMin)) / (oldMax - oldMin)) + newMin;


    return TransferValues(intermediateValue, newMax) * intermediateValue;
}

scm::math::mat4f const Camera::
GetViewMatrix() const
{
    switch (cam_state_)
    {
        case CAM_STATE_LAMURE:
            return scm::math::mat4f(trackball_.transform());
            break;

        case CAM_STATE_GUA:
            return view_matrix_;
            break;

        default: break;
    }

    return scm::math::mat4f();
}

scm::math::mat4d const Camera::
GetHighPrecisionViewMatrix() const {
  
  if (cam_state_ == CAM_STATE_LAMURE) {
     return trackball_.transform();
  }

  return scm::math::mat4d(view_matrix_);

}

scm::math::mat4f const Camera::
GetProjectionMatrix() const
{
    return projection_matrix_;
}

std::vector<scm::math::vec3d> Camera::get_frustum_corners() const
{
  std::vector<scm::math::vec4d> tmp(8);
  std::vector<scm::math::vec3d> result(8);

  scm::math::mat4d inverse_transform;

  if(CAM_STATE_LAMURE == cam_state_) {
      inverse_transform = scm::math::inverse(scm::math::mat4d(projection_matrix_) * trackball_.transform());
  }
  else if(CAM_STATE_GUA == cam_state_) {
      inverse_transform = scm::math::mat4d(scm::math::inverse(projection_matrix_ * view_matrix_));
  }

  tmp[0] = inverse_transform * scm::math::vec4d(-1, -1, -1, 1);
  tmp[1] = inverse_transform * scm::math::vec4d(-1, -1,  1, 1);
  tmp[2] = inverse_transform * scm::math::vec4d(-1,  1, -1, 1);
  tmp[3] = inverse_transform * scm::math::vec4d(-1,  1,  1, 1);
  tmp[4] = inverse_transform * scm::math::vec4d( 1, -1, -1, 1);
  tmp[5] = inverse_transform * scm::math::vec4d( 1, -1,  1, 1);
  tmp[6] = inverse_transform * scm::math::vec4d( 1,  1, -1, 1);
  tmp[7] = inverse_transform * scm::math::vec4d( 1,  1,  1, 1);

  for (int i(0); i<8; ++i) {
    result[i] = tmp[i]/tmp[i][3];
  }

  return result;

}


}

}
