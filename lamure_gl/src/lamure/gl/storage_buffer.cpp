// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/gl/storage_buffer.h>

namespace lamure {
namespace gl {


storage_buffer_t::storage_buffer_t(
  uint32_t num_elements,
  uint32_t element_size)
: buffer_t(num_elements, element_size, element_size/sizeof(uint32_t))
{
  glGenBuffers(1, &buffer_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_);

  glBufferData(GL_SHADER_STORAGE_BUFFER, get_size(), 0, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}


storage_buffer_t::~storage_buffer_t()
{
  glDeleteBuffers(1, &buffer_);
}

void*
storage_buffer_t::map(
  GLuint map_mode)
{
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_);
  void* mapped = (void*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, map_mode);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  return mapped;
}

void
storage_buffer_t::unmap()
{
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_);
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void
storage_buffer_t::enable(
  uint32_t slot)
{
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, buffer_);
}

void
storage_buffer_t::disable()
{

}

} // namespace gl
} // namespace lamure

