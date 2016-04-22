
#include <lamure/gl/array_buffer.h>

namespace lamure {
namespace gl {

inline
array_buffer_t::array_buffer_t(
  uint32_t num_vertices,
  uint32_t vertex_size)
: buffer_t(num_vertices, vertex_size, vertex_size/sizeof(float32_t))
{
  glGenBuffers(1, &buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, buffer_);

  glBufferData(GL_ARRAY_BUFFER, get_size(), 0, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

inline
array_buffer_t::~array_buffer_t()
{
  glDeleteBuffers(1, &buffer_);
}

inline void*
array_buffer_t::map(GLuint map_mode)
{
  glBindBuffer(GL_ARRAY_BUFFER, buffer_);
  void* mapped = (void*)glMapBuffer(GL_ARRAY_BUFFER, map_mode);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  return mapped;
}

inline void
array_buffer_t::unmap()
{
  glBindBuffer(GL_ARRAY_BUFFER, buffer_);
  glUnmapBuffer(GL_ARRAY_BUFFER);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}


} // namespace gl
} // namespace lamure
