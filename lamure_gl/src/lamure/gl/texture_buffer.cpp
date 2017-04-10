// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/gl/texture_buffer.h>

namespace lamure {
namespace gl {


texture_buffer_t::texture_buffer_t(
  uint32_t num_elements,
  uint32_t element_size,
  GLenum internal_format)
: buffer_t(num_elements, element_size, element_size/sizeof(uint32_t)),
  internal_format_(internal_format)
{
  glGenBuffers(1, &buffer_);
  glBindBuffer(GL_TEXTURE_BUFFER, buffer_);
  glBufferData(GL_TEXTURE_BUFFER, get_size(), 0, GL_STATIC_DRAW);

  glGenTextures(1, &texture_);
  glBindBuffer(GL_TEXTURE_BUFFER, 0);
}


texture_buffer_t::~texture_buffer_t()
{
  glDeleteBuffers(1, &buffer_);
  glDeleteTextures(1, &texture_);
}

void*
texture_buffer_t::map(
  GLuint map_mode)
{
  glBindBuffer(GL_TEXTURE_BUFFER, buffer_);
  void* mapped = (void*)glMapBuffer(GL_TEXTURE_BUFFER, map_mode);
  glBindBuffer(GL_TEXTURE_BUFFER, 0);
  return mapped;
}

void
texture_buffer_t::unmap()
{
  glBindBuffer(GL_TEXTURE_BUFFER, buffer_);
  glUnmapBuffer(GL_TEXTURE_BUFFER);
  glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

void
texture_buffer_t::bind_texture()
{
  glBindTexture(GL_TEXTURE_BUFFER, texture_);
  glTexBuffer(GL_TEXTURE_BUFFER, internal_format_, buffer_);
}

} // namespace gl
} // namespace lamure
