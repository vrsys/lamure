
#include <lamure/gl/buffer.h>

namespace lamure {
namespace gl {

inline
buffer_t::buffer_t(
  uint32_t num_elements,
  uint32_t element_size,
  uint32_t element_components)
: num_elements_(num_elements),
  element_size_(element_size),
  element_components_(element_components)
{

}

inline
buffer_t::~buffer_t()
{

}

inline uint32_t
buffer_t::get_num_elements() const
{
  return num_elements_;
}

inline uint32_t
buffer_t::get_element_size() const
{
  return element_size_;
}

inline uint32_t
buffer_t::get_element_components() const
{
  return element_components_;
}

inline uint32_t
buffer_t::get_size() const
{
  return num_elements_*element_size_;
}

inline const GLuint
buffer_t::get_buffer() const
{
  return buffer_;
}

inline void
buffer_t::set_data(
  const char* data,
  size_t length)
{
  LAMURE_ASSERT(length <= get_size(), "length exceeds buffer size");
  LAMURE_ASSERT(length > 0, "length is zero");
  char* mapped_buffer = (char*)map(GL_WRITE_ONLY);
  memcpy(mapped_buffer, (char*)data, length);
  unmap();
}

inline void
buffer_t::set_zero()
{
  char* mapped_buffer = (char*)map(GL_WRITE_ONLY);
  memset(mapped_buffer, 0, get_size());
  unmap();
}

} // namespace gl
} // namespace lamure
