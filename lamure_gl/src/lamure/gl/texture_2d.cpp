// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/gl/texture_2d.h>

namespace lamure {
namespace gl {


texture_2d_t::texture_2d_t(
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


texture_2d_t::texture_2d_t(
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
texture_2d_t::create()
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


texture_2d_t::~texture_2d_t()
{
  glDeleteBuffers(1, &buffer_);
}

void
texture_2d_t::set_pixels(
  char* data)
{
  LAMURE_ASSERT(data != nullptr, "nullptr encountered");
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, internal_format_, width_, height_, 0, format_, internal_type_, data);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void
texture_2d_t::get_pixels(
  char** data)
{
  LAMURE_ASSERT(*data == nullptr, "data must be nullptr");
  *data = new char[bytes_per_pixel_*width_*height_];

  glBindTexture(GL_TEXTURE_2D, texture_);
  glGetTexImage(GL_TEXTURE_2D, 0, format_, internal_type_, *data);
  glBindTexture(GL_TEXTURE_2D, 0);

}

void
texture_2d_t::enable(
  uint32_t slot,
  GLenum access)
{
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, texture_);
}

void texture_2d_t::
disable() {
  glBindTexture(GL_TEXTURE_2D, 0);
}

void
texture_2d_t::draw()
{
  glBindTexture(GL_TEXTURE_2D, texture_);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
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
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glDisable(GL_TEXTURE_2D);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);

  glBindTexture(GL_TEXTURE_2D, 0);
}

uint32_t
texture_2d_t::get_num_elements() const
{
  return num_elements_;
}

uint32_t
texture_2d_t::get_element_size() const
{
  return element_size_;
}

uint32_t
texture_2d_t::get_size() const
{
  return element_size_*num_elements_;
}

uint32_t
texture_2d_t::get_width() const
{
  return width_;
}

uint32_t
texture_2d_t::get_height() const
{
  return height_;
}

} // namespace gl
} // namespace lamure
