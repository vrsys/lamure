
#ifndef LAMURE_GL_ARRAY_BUFFER_H_
#define LAMURE_GL_ARRAY_BUFFER_H_

#include <lamure/gl/buffer.h>
#include <lamure/platform_gl.h>

namespace lamure {
namespace gl {

class array_buffer_t : public buffer_t {
public:
  array_buffer_t(uint32_t num_vertices, uint32_t vertex_size);
  ~array_buffer_t();

  void* map(GLuint map_mode);
  void unmap();

private:

};

} // namespace gl
} // namespace lamure

#endif
