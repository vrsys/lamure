
#ifndef LAMURE_GL_BUFFER_H_
#define LAMURE_GL_BUFFER_H_

#include <lamure/types.h>
#include <lamure/platform_gl.h>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <cstring>


namespace lamure {
namespace gl {

class buffer_t {
public:
  virtual ~buffer_t();

  uint32_t get_num_elements() const;
  uint32_t get_element_size() const;
  uint32_t get_element_components() const;
  uint32_t get_size() const;
    
  const GLuint get_buffer() const;

  virtual void* map(GLuint map_mode) = 0;
  virtual void unmap() = 0;

  void set_data(const char* data, size_t length);
  void set_zero();

protected:
  buffer_t(uint32_t num_elements, uint32_t element_size, uint32_t element_components);

  uint32_t num_elements_;
  uint32_t element_size_;
  uint32_t element_components_;

  GLuint buffer_;

private:

};

} // namespace gl
} // namespace lamure

#endif
