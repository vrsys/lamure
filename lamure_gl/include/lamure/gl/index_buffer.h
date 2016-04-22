
#ifndef LAMURE_GL_INDEX_BUFFER_H_
#define LAMURE_GL_INDEX_BUFFER_H_

#include <lamure/gl/buffer.h>
#include <lamure/platform_gl.h>

namespace lamure {
namespace gl {

class index_buffer_t : public buffer_t {
public:
  index_buffer_t(uint32_t num_indices);
  virtual ~index_buffer_t();

  void* map(GLuint map_mode);
  void unmap();

private:

};

} // namespace gl
} // namespace lamure

#endif
