
#ifndef LAMURE_GL_FRAME_BUFFER_H_
#define LAMURE_GL_FRAME_BUFFER_H_

#include <lamure/types.h>
#include <lamure/assert.h>
#include <lamure/platform_gl.h>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <vector>

namespace lamure {
namespace gl {

class frame_buffer_t
{
public:
  frame_buffer_t(uint32_t num_buffers, uint32_t width, uint32_t height, GLenum format, GLenum interpolation);
  ~frame_buffer_t();

  void enable();
  void disable();
  void draw(uint32_t index);
  void get_pixels(uint32_t index, char** data);

  void bind_texture(uint32_t index);
  void unbind_texture(uint32_t index);

  uint32_t get_width() const;
  uint32_t get_height() const;

protected:
  uint32_t width_;
  uint32_t height_;
  uint32_t num_buffers_;

  std::vector<GLuint>	textures_;

private:
  GLenum layout_[16];
  GLuint buffer_;
  GLuint depth_;
  GLenum format_;
};

} // namespace gl
} // namespace lamure

#endif
