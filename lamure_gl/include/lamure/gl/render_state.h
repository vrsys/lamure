
#ifndef LAMURE_GL_RENDER_STATE_H_
#define LAMURE_GL_RENDER_STATE_H_

namespace lamure {
namespace gl {

class render_state_t {
public:
  render_state_t();
  ~render_state_t();

  enum compare_func_t {
    COMPARISON_NEVER = 0x00,
    COMPARISON_ALWAYS,
    COMPARISON_LESS,
    COMPARISON_LESS_EQUAL,
    COMPARISON_EQUAL,
    COMPARISON_GREATER,
    COMPARISON_GREATER_EQUAL,
    COMPARISON_NOT_EQUAL
  };
  
  enum fill_mode_t {
    FILL_SOLID = 0x00,
    FILL_WIREFRAME,
    FILL_POINT
  };
  
  enum cull_mode_t {
    CULL_NONE = 0x00,
    CULL_FRONT,
    CULL_BACK
  };

  enum polygon_orientation_t {
    ORIENT_CW = 0x00,
    ORIENT_CCW
  };
  
  enum blend_func_t {
    FUNC_ZERO = 0x00,
    FUNC_ONE,
    FUNC_SRC_COLOR,
    FUNC_ONE_MINUS_SRC_COLOR,
    FUNC_DST_COLOR,
    FUNC_ONE_MINUS_DST_COLOR,
    FUNC_SRC_ALPHA,
    FUNC_ONE_MINUS_SRC_ALPHA,
    FUNC_DST_ALPHA,
    FUNC_ONE_MINUS_DST_ALPHA,
    FUNC_CONSTANT_COLOR,
    FUNC_ONE_MINUS_CONSTANT_COLOR,
    FUNC_CONSTANT_ALPHA,
    FUNC_ONE_MINUS_CONSTANT_ALPHA,
    FUNC_SRC_ALPHA_SATURATE,
    FUNC_SRC1_COLOR,
    FUNC_ONE_MINUS_SRC1_COLOR,
    FUNC_SRC1_ALPHA,
    FUNC_ONE_MINUS_SRC1_ALPHA
  };

  enum blend_equation_t {
    EQ_FUNC_ADD = 0x00,
    EQ_FUNC_SUBTRACT,
    EQ_FUNC_REVERSE_SUBTRACT,
    EQ_MIN,
    EQ_MAX
  };

  enum texture_filter_mode_t {
    FILTER_MIN_MAG_NEAREST = 0x00,
    FILTER_MIN_NEAREST_MAG_LINEAR,
    FILTER_MIN_LINEAR_MAG_NEAREST,
    FILTER_MIN_MAG_LINEAR,
    FILTER_MIN_MAG_MIP_NEAREST,
    FILTER_MIN_MAG_NEAREST_MIP_LINEAR,
    FILTER_MIN_NEAREST_MAG_LINEAR_MIP_NEAREST,
    FILTER_MIN_NEAREST_MAG_MIP_LINEAR,
    FILTER_MIN_LINEAR_MAG_MIP_NEAREST,
    FILTER_MIN_LINEAR_MAG_NEAREST_MIP_LINEAR,
    FILTER_MIN_MAG_LINEAR_MIP_NEAREST,
    FILTER_MIN_MAG_MIP_LINEAR,
    FILTER_ANISOTROPIC,
  };

  enum texture_wrap_mode_t {
    WRAP_CLAMP_TO_EDGE  = 0x00,
    WRAP_REPEAT,
    WRAP_MIRRORED_REPEAT
  };

  
  void set_depth_stencil_state(bool in_depth_test, 
                               bool in_depth_mask, 
                               compare_func_t in_depth_func);
                          
  void set_rasterizer_state(fill_mode_t in_fmode, 
                            cull_mode_t in_cmode, 
                            polygon_orientation_t in_fface,
                            bool in_msample,  
                            bool in_sshading, 
                            float in_min_sshading,
                            bool in_sctest, 
                            bool in_smlines);
                          
  void set_blend_state(bool in_enabled,
                       blend_func_t in_src_rgb_func, 
                       blend_func_t in_dst_rgb_func,
                       blend_func_t in_src_alpha_func, 
                       blend_func_t in_dst_alpha_func,
                       blend_equation_t in_rgb_equation, 
                       blend_equation_t in_alpha_equation);
                     
  void set_sampler_state(texture_filter_mode_t  in_filter,
                         texture_wrap_mode_t    in_wrap);


private:


};

} // namespace lamure
} // namespace gl

#endif

