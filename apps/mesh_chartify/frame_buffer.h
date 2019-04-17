#ifndef FRAME_BUFFER_H_
#define FRAME_BUFFER_H_

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <vector>

class frame_buffer_t {
public:
  frame_buffer_t();
  frame_buffer_t(uint32_t num_buffers, uint32_t width, uint32_t height, GLenum format, GLenum interpolation);
  ~frame_buffer_t();

  void enable();
  void disable();
  void draw(uint32_t index);
  void get_pixels(uint32_t index, char** data);
  void get_pixels(uint32_t index, std::vector<uint8_t>& image);

  float read_depth(uint32_t x, uint32_t y);

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


#endif
