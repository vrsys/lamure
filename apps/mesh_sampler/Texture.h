// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef TEXTURE_H_
#define TEXTURE_H_

#include <string>
#include <vector>
#include <memory>

class fipImage;

class Texture
{
public:

  struct Pixel {
    unsigned char r;
    unsigned char g;
    unsigned char b;
  };

  Texture() = default;
  Texture(Texture&& other);
  Texture& operator=(Texture&& other);

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;

  bool Load(const std::string& filename);
  bool IsLoaded() const { return _image != nullptr; }

  int Width() const;
  int Height() const;

  Pixel GetPixel(const int x, const int y) const;
  Pixel GetPixel(const double x, const double y) const;

private:

  std::shared_ptr<fipImage> _image = nullptr;

};

#endif // TEXTURE_H_

