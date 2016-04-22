
#include <lamure/gl/storage_buffer.h>

namespace lamure {
namespace gl {

inline
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

inline
storage_buffer_t::~storage_buffer_t()
{
  glDeleteBuffers(1, &buffer_);
}

inline void*
storage_buffer_t::map(
  GLuint map_mode)
{
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_);
  void* mapped = (void*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, map_mode);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  return mapped;
}

inline void
storage_buffer_t::unmap()
{
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_);
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

inline void
storage_buffer_t::enable(
  uint32_t slot)
{
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, buffer_);
}

inline void
storage_buffer_t::disable()
{

}

} // namespace gl
} // namespace lamure

