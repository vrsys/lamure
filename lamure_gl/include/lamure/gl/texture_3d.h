// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_GL_TEXTURE_3D_H_
#define LAMURE_GL_TEXTURE_3D_H_

#include <lamure/types.h>
#include <lamure/platform_gl.h>

#include <GL/glew.h>
#include <GL/freeglut.h>

namespace lamure {
namespace gl {

class texture_3d_t {
public:
  texture_3d_t(uint32_t width, uint32_t height, uint32_t depth, 
    GLenum internal_format, GLenum format, GLenum internal_type, GLenum interpolation);
  ~texture_3d_t();

  void set_pixels(char* data);
  void get_pixels(char** data);

  uint32_t get_num_elements() const;
  uint32_t get_element_size() const;
  uint32_t get_size() const;

  void enable(uint32_t slot, GLenum access);
  void disable();

  uint32_t get_width() const;
  uint32_t get_height() const;
  uint32_t get_depth() const;

protected:
  uint32_t width_;
  uint32_t height_;
  uint32_t depth_;
  uint32_t bytes_per_pixel_;

  uint32_t num_elements_;
  uint32_t element_size_;
  uint32_t num_bytes_;
  
  GLuint buffer_;
  GLuint texture_;
  GLuint internal_format_;
  GLuint format_;
  GLuint internal_type_;
  GLuint interpolation_;

  void create();

private:
 
};

} // namespace gl
} // namespace lamure

#endif
