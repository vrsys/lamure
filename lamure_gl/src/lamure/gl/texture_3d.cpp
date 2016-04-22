
#include <lamure/gl/texture_3d.h>

namespace lamure {
namespace gl {

inline
texture_3d_t::texture_3d_t(
  uint32_t width,
  uint32_t height,
  uint32_t depth, 
  GLenum internal_format,
  GLenum format,
  GLenum internal_type,
  GLenum interpolation)
: width_(width),
  height_(height),
  depth_(depth),
  bytes_per_pixel_(8), 
  internal_format_(internal_format),
  format_(format),
  internal_type_(internal_type),
  interpolation_(interpolation)
{
  create();
}


inline void
texture_3d_t::create()
{
  uint32_t size = width_ * height_ * depth_ * sizeof(GLubyte) * bytes_per_pixel_;

  glGenBuffers(1, &buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, buffer_);
  glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_3D, texture_);

  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, interpolation_);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, interpolation_);

  
  glTexImage3D(GL_TEXTURE_3D, 0, internal_format_, width_, height_, depth_, 0, format_, internal_type_, NULL);
  glBindTexture(GL_TEXTURE_3D, 0);

}

inline
texture_3d_t::~texture_3d_t()
{
  glDeleteBuffers(1, &buffer_);
}

inline void
texture_3d_t::set_pixels(
  char* data)
{
  LAMURE_ASSERT(data != nullptr, "nullptr encountered");
  glBindTexture(GL_TEXTURE_3D, texture_);
  glTexImage3D(GL_TEXTURE_3D, 0, internal_format_, width_, height_, depth_, 0, format_, internal_type_, data);
  glBindTexture(GL_TEXTURE_3D, 0);
}

inline void
texture_3d_t::get_pixels(
  char** data)
{
  LAMURE_ASSERT(*data == nullptr, "data must be nullptr");
  *data = new char[bytes_per_pixel_*width_*height_*depth_];

  glBindTexture(GL_TEXTURE_3D, texture_);
  glGetTexImage(GL_TEXTURE_3D, 0, format_, internal_type_, *data);
  glBindTexture(GL_TEXTURE_3D, 0);

}

inline void
texture_3d_t::enable(
  uint32_t slot,
  GLenum access)
{
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_3D, texture_);
}

inline void
texture_3d_t::disable()
{
  glBindTexture(GL_TEXTURE_3D, 0);
}

inline uint32_t
texture_3d_t::get_num_elements() const
{
  return num_elements_;
}

inline uint32_t
texture_3d_t::get_element_size() const
{
  return element_size_;
}

inline uint32_t
texture_3d_t::get_size() const
{
  return element_size_*num_elements_;
}

inline uint32_t
texture_3d_t::get_width() const
{
  return width_;
}

inline uint32_t
texture_3d_t::get_height() const
{
  return height_;
}

inline uint32_t
texture_3d_t::get_depth() const
{
  return depth_;
}

} // namespace gl
} // namespace lamure
