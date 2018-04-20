// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
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

class texture
{
public:

  struct pixel {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
  };

  texture() = default;
  texture(texture&& other);
  texture& operator=(texture&& other);

  texture(const texture&) = delete;
  texture& operator=(const texture&) = delete;

  bool load(const std::string& filename);
  bool is_loaded() const { return _image != nullptr; }

  int width() const;
  int height() const;

  pixel get_pixel(const int x, const int y) const;
  pixel get_pixel(const double x, const double y) const;

private:

  std::shared_ptr<fipImage> _image = nullptr;

};

#endif // TEXTURE_H_

