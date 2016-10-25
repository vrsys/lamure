
#include <lamure/gl/render_state.h>

namespace lamure {
namespace gl {

render_state_t::render_state_t() {

}

render_state_t::~render_state_t() {

}



void render_state_t::
set_depth_stencil_state(
  bool in_depth_test, 
  bool in_depth_mask, 
  compare_func_t in_depth_func) {
                             
                             
}
                        
void render_state_t::
set_rasterizer_state(
  fill_mode_t in_fmode, 
  cull_mode_t in_cmode, 
  polygon_orientation_t in_fface,
  bool in_msample,  
  bool in_sshading, 
  float in_min_sshading,
  bool in_sctest, 
  bool in_smlines) {
                        
}

void render_state_t::
set_blend_state(
  bool in_enabled,
  blend_func_t in_src_rgb_func, 
  blend_func_t in_dst_rgb_func,
  blend_func_t in_src_alpha_func, 
  blend_func_t in_dst_alpha_func,
  blend_equation_t in_rgb_equation, 
  blend_equation_t in_alpha_equation) {
                     
}
                   
void render_state_t::
set_sampler_state(
  texture_filter_mode_t in_filter,
  texture_wrap_mode_t in_wrap) {
                       
}                       












} // namespace lamure
} // namespace gl

