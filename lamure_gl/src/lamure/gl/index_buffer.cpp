
#include <lamure/gl/index_buffer.h>

namespace lamure {
namespace gl {

inline
index_buffer_t::index_buffer_t(
  uint32_t num_indices)
: buffer_t(num_indices, sizeof(uint32_t), 1)
{
  glGenBuffers(1, &buffer_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_);

  glBufferData(GL_ELEMENT_ARRAY_BUFFER, get_size(), 0, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

inline
index_buffer_t::~index_buffer_t()
{
  glDeleteBuffers(1, &buffer_);
}

inline void*
index_buffer_t::map(
  GLuint map_mode)
{
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_);
  void* mapped = (void*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, map_mode);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  return mapped;
}

inline void
index_buffer_t::unmap()
{
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer_);
  glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

} // namespace gl
} // namespace lamure
