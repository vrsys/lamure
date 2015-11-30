// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "texture.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <memory>

#include <FreeImagePlus.h>

texture::texture(texture&& other)
: _image(other._image)
{
  other._image.reset();
}

texture& texture::
operator=(texture&& other)
{
  if (this != &other)
  {
    _image = other._image;
    other._image = nullptr;
  }
  return *this;
}

bool texture::
load(const std::string& filename)
{
  _image = std::make_shared<fipImage>();

  if (!_image->load(filename.c_str()))
  {
    std::cerr << "texture::load(): " << "unable to open file: " << filename << std::endl;
    return false;
  }
  else {
    std::cerr << "texture::load(): " << filename << " : " << _image->getWidth() << "x" << _image->getHeight() << std::endl;
  }

  return true;
}

int texture::
width() const
{
  assert(is_loaded());
  return _image->getWidth();
}

int texture::
height() const
{
  assert(is_loaded());
  return _image->getHeight();
}

texture::pixel texture::
get_pixel(const int x, const int y) const
{
  assert(is_loaded());
  pixel outpixel = { 0, 0, 0 };

  // wrap coords
  int posX = (x >= 0) ? (x % _image->getWidth()) :
    (_image->getWidth() + (x%_image->getWidth())) % _image->getWidth();
  int posY = (y >= 0) ? (y % _image->getHeight()) :
    (_image->getHeight() + (y%_image->getHeight())) % _image->getHeight();

  RGBQUAD pixel;
  _image->getpixelColor(x, y, &pixel);
  outpixel = { pixel.rgbRed, pixel.rgbGreen, pixel.rgbBlue };

  return outpixel;
}

texture::pixel texture::
get_pixel(const double x, const double y) const
{
  pixel outpixel = { 0, 0, 0 };
  int xF = floor(x);
  int yF = floor(y);
  int xC = ceil(x);
  int yC = ceil(y);

  pixel c00 = get_pixel(xF, yC);
  pixel c01 = get_pixel(xF, yF);
  pixel c10 = get_pixel(xC, yC);
  pixel c11 = get_pixel(xC, yF);

  double tx = x - double(xF);
  double ty = 1.0 - (y - double(yF));
  double a, b;

  a = double(c00.r) * (1.0 - tx) + double(c10.r) * tx;
  b = double(c01.r) * (1.0 - tx) + double(c11.r) * tx;
  outpixel.r = a * (1.0 - ty) + b * ty;

  a = double(c00.g) * (1.0 - tx) + double(c10.g) * tx;
  b = double(c01.g) * (1.0 - tx) + double(c11.g) * tx;
  outpixel.g = a * (1.0 - ty) + b * ty;

  a = double(c00.b) * (1.0 - tx) + double(c10.b) * tx;
  b = double(c01.b) * (1.0 - tx) + double(c11.b) * tx;
  outpixel.b = a * (1.0 - ty) + b * ty;

  return outpixel;
}