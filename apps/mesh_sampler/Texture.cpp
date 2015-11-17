// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "Texture.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <memory>

#include <FreeImagePlus.h>

Texture::Texture(Texture&& other)
: _image(other._image)
{
  other._image.reset();
}

Texture& Texture::
operator=(Texture&& other)
{
  if (this != &other)
  {
    _image = other._image;
    other._image = nullptr;
  }
  return *this;
}

bool Texture::
Load(const std::string& filename)
{
  _image = std::make_shared<fipImage>();

  if (!_image->load(filename.c_str()))
  {
    std::cerr << "Texture::load(): " << "unable to open file: " << filename << std::endl;
    return false;
  }
  else {
    std::cerr << "Texture::load(): " << filename << " : " << _image->getWidth() << "x" << _image->getHeight() << std::endl;
  }

  return true;
}

int Texture::
Width() const
{
  assert(IsLoaded());
  return _image->getWidth();
}

int Texture::
Height() const
{
  assert(IsLoaded());
  return _image->getHeight();
}

Texture::Pixel Texture::
GetPixel(const int x, const int y) const
{
  assert(IsLoaded());
  Pixel outPixel = { 0, 0, 0 };

  // wrap coords
  int posX = (x >= 0) ? (x % _image->getWidth()) :
    (_image->getWidth() + (x%_image->getWidth())) % _image->getWidth();
  int posY = (y >= 0) ? (y % _image->getHeight()) :
    (_image->getHeight() + (y%_image->getHeight())) % _image->getHeight();

  RGBQUAD pixel;
  _image->getPixelColor(x, y, &pixel);
  outPixel = { pixel.rgbRed, pixel.rgbGreen, pixel.rgbBlue };

  return outPixel;
}

Texture::Pixel Texture::
GetPixel(const double x, const double y) const
{
  Pixel outPixel = { 0, 0, 0 };
  int xF = floor(x);
  int yF = floor(y);
  int xC = ceil(x);
  int yC = ceil(y);

  Pixel c00 = GetPixel(xF, yC);
  Pixel c01 = GetPixel(xF, yF);
  Pixel c10 = GetPixel(xC, yC);
  Pixel c11 = GetPixel(xC, yF);

  double tx = x - double(xF);
  double ty = 1.0 - (y - double(yF));
  double a, b;

  a = double(c00.r) * (1.0 - tx) + double(c10.r) * tx;
  b = double(c01.r) * (1.0 - tx) + double(c11.r) * tx;
  outPixel.r = a * (1.0 - ty) + b * ty;

  a = double(c00.g) * (1.0 - tx) + double(c10.g) * tx;
  b = double(c01.g) * (1.0 - tx) + double(c11.g) * tx;
  outPixel.g = a * (1.0 - ty) + b * ty;

  a = double(c00.b) * (1.0 - tx) + double(c10.b) * tx;
  b = double(c01.b) * (1.0 - tx) + double(c11.b) * tx;
  outPixel.b = a * (1.0 - ty) + b * ty;

  return outPixel;
}