
#include "texture.h"

texture_t::texture_t()
: width_(0),
  height_(0),
  bytes_per_pixel_(4), 
  internal_format_(GL_RGBA8),
  format_(GL_RGBA),
  internal_type_(GL_UNSIGNED_BYTE),
  interpolation_(GL_NEAREST) {

}

texture_t::texture_t(
  uint32_t width,
  uint32_t height,
  GLenum interpolation)
: width_(width),
  height_(height),
  bytes_per_pixel_(4), 
  internal_format_(GL_RGBA8),
  format_(GL_RGBA),
  internal_type_(GL_UNSIGNED_BYTE),
  interpolation_(interpolation)
{
  create();
}

texture_t::texture_t(
  uint32_t width,
  uint32_t height, 
  GLenum internal_format,
  GLenum format,
  GLenum internal_type,
  GLenum interpolation)
: width_(width),
  height_(height),
  bytes_per_pixel_(8), 
  internal_format_(internal_format),
  format_(format),
  internal_type_(internal_type),
  interpolation_(interpolation)
{
  create();
}

void
texture_t::create()
{
  uint32_t size = width_ * height_ * sizeof(GLubyte) * bytes_per_pixel_;

  glGenBuffers(1, &buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, buffer_);
  glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation_);

  glTexImage2D(GL_TEXTURE_2D, 0, internal_format_, width_, height_, 0, format_, internal_type_, NULL);
}

texture_t::~texture_t()
{
  glDeleteBuffers(1, &buffer_);
}

void
texture_t::set_pixels(
  GLubyte* data)
{
  if (data == nullptr) {
    std::cout << "nullptr encountered" << std::endl;
  }
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, internal_format_, width_, height_, 0, format_, internal_type_, data);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void
texture_t::enable(
  uint32_t slot)
{
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, texture_);
}

void texture_t::
disable() {
  glBindTexture(GL_TEXTURE_2D, 0);
}

void
texture_t::draw(float _angle)
{
  glBindTexture(GL_TEXTURE_2D, texture_);

  glDisable(GL_DEPTH_TEST);
  //glDepthMask(GL_FALSE);
  glDisable(GL_LIGHTING);
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glRotatef(_angle, 0.0, 0.0, 1.0);

  glBegin(GL_QUADS);

  glTexCoord2f(0.0, 0.0); glVertex3f(-1.0, -1.0, 0.0);
  glTexCoord2f(1.0, 0.0); glVertex3f(1.0, -1.0, 0.0);
  glTexCoord2f(1.0, 1.0); glVertex3f(1.0, 1.0, 0.0);
  glTexCoord2f(0.0, 1.0); glVertex3f(-1.0, 1.0, 0.0);

  glEnd();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glDisable(GL_TEXTURE_2D);
  //glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);

  glBindTexture(GL_TEXTURE_2D, 0);
}

uint32_t
texture_t::get_num_elements() const
{
  return num_elements_;
}

uint32_t
texture_t::get_element_size() const
{
  return element_size_;
}

uint32_t
texture_t::get_size() const
{
  return element_size_*num_elements_;
}

uint32_t
texture_t::get_width() const
{
  return width_;
}

uint32_t
texture_t::get_height() const
{
  return height_;
}

