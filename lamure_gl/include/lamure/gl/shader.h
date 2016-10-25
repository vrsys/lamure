// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_GL_SHADER_H_
#define LAMURE_GL_SHADER_H_

#include <lamure/math.h>
#include <lamure/assert.h>
#include <lamure/config.h>
#include <lamure/platform_gl.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <lamure/gl/frame_buffer.h>
#include <lamure/gl/storage_buffer.h>
#include <lamure/gl/texture_buffer.h>
#include <lamure/gl/texture_2d.h>
#include <lamure/gl/texture_3d.h>

namespace lamure {
namespace gl {

class shader_t {
public:
  shader_t();
  ~shader_t();

  void  attach(GLenum type, const std::string& filename);
  void  link();

  GLuint get_program() const;

  void  enable();
  void  disable();

  void  set(const std::string& uniform, const lamure::math::mat4f_t& matrix);
  void  set(const std::string& uniform, const lamure::math::vec2f_t& vector);
  void  set(const std::string& uniform, const lamure::math::vec3f_t& vector);
  void  set(const std::string& uniform, const int32_t value);
  void  set(const std::string& uniform, const float32_t value);
  void  set(const std::string& uniform, uint32_t slot, frame_buffer_t* frame_buffer);
  void  set(const std::string& uniform, uint32_t slot, storage_buffer_t* storage_buffer);
  void  set(const std::string& uniform, uint32_t slot, texture_buffer_t* texture_buffer);
  void  set(const std::string& uniform, uint32_t slot, texture_2d_t* texture);
  void  set(const std::string& uniform, uint32_t slot, texture_3d_t* texture);

private:
  static GLuint load(GLenum shader_type, const std::string& filename);
  static std::string load_file(const std::string& filename);

  std::vector<GLuint> shaders_;
  GLuint  program_;
  bool linked_;

};

} // namespace gl
} // namespace lamure

#endif

