
#include <lamure/gl/frame_buffer.h>

namespace lamure {
namespace gl {

inline
frame_buffer_t::frame_buffer_t(
  uint32_t num_buffers,
  uint32_t width,
  uint32_t height,
  GLenum format,
  GLenum interpolation)
: width_(width),
  height_(height),
  num_buffers_(num_buffers),
  format_(format)
{
  LAMURE_ASSERT(num_buffers_ < 17, "num framebuffers must not exceed 16");

  for (uint32_t i = 0; i < num_buffers_; ++i) {
    textures_.push_back(0);
    glGenTextures(1, &textures_[i]);
    glBindTexture(GL_TEXTURE_2D, textures_[i]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format_, width_, height_, 0, format_, GL_UNSIGNED_BYTE, (void*)0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  glGenRenderbuffers(1, &depth_);

  glBindRenderbuffer(GL_RENDERBUFFER, depth_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width_, height_);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  glGenFramebuffers(1, &buffer_);
  glBindFramebuffer(GL_FRAMEBUFFER, buffer_);

  uint32_t attachment = 0x8ce0;
  for (uint32_t i = 0; i < num_buffers_; i++) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, textures_[i], 0);
    layout_[i] = attachment;
    attachment++;
  }

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_);
  LAMURE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "error during framebuffer setup");
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

inline
frame_buffer_t::~frame_buffer_t()
{
  for (uint32_t i = 0; i < num_buffers_; i++) {
    glDeleteTextures(1, &textures_[i]);
  }

  glDeleteRenderbuffers(1, &depth_);
  glDeleteFramebuffers(1, &buffer_);
}

inline void
frame_buffer_t::enable()
{
  glBindFramebuffer(GL_FRAMEBUFFER, buffer_);
  glDrawBuffers(num_buffers_, layout_);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

inline void
frame_buffer_t::disable()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

inline void
frame_buffer_t::bind_texture(uint32_t index)
{
  LAMURE_ASSERT(index < num_buffers_, "index " + lamure::util::to_string(index) + "is out of bounds");
  glActiveTexture(GL_TEXTURE0 + index);
  glBindTexture(GL_TEXTURE_2D, textures_[index]);
}

inline void
frame_buffer_t::unbind_texture(
  uint32_t index)
{
  LAMURE_ASSERT(index < num_buffers_, "index " + lamure::util::to_string(index) + "is out of bounds");
  glActiveTexture(GL_TEXTURE0 + index);
  glBindTexture(GL_TEXTURE_2D, 0);
}

inline uint32_t
frame_buffer_t::get_width() const
{
  return width_;
}

inline uint32_t
frame_buffer_t::get_height() const
{
  return height_;
}

inline void
frame_buffer_t::draw(
  uint32_t index)
{
  LAMURE_ASSERT(index < num_buffers_, "index " + lamure::util::to_string(index) + " is out of bounds");

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textures_[index]);

  //glViewport(0, 0, width_, height_);

  //glDisable(GL_DEPTH_TEST);
  //glDisable(GL_LIGHTING);
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glBegin(GL_QUADS);

  glTexCoord2f(0.0, 0.0); glVertex3f(-1.0, -1.0, 0.0);
  glTexCoord2f(1.0, 0.0); glVertex3f(1.0, -1.0, 0.0);
  glTexCoord2f(1.0, 1.0); glVertex3f(1.0, 1.0, 0.0);
  glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, 1.0, 0.0);

  glEnd();

  //glDisable(GL_TEXTURE_2D);
  //glEnable(GL_DEPTH_TEST);
  //glEnable(GL_LIGHTING);

  glBindTexture(GL_TEXTURE_2D, 0);
}

inline void
frame_buffer_t::get_pixels(
  uint32_t index,
  char** data)
{
  LAMURE_ASSERT(index < num_buffers_, "index " + lamure::util::to_string(index) + " is out of bounds");

  LAMURE_ASSERT(*data == nullptr, "data must be nullptr");
  *data = new char[4*width_*height_];

  glBindTexture(GL_TEXTURE_2D, textures_[index]);
  glGetTexImage(GL_TEXTURE_2D, 0, format_, GL_UNSIGNED_BYTE, *data);
  glBindTexture(GL_TEXTURE_2D, 0);

}


} // namespace gl
} // namespace lamure
