#include "Image.h"

int Image::get_height() const {
  return _height;
}

void Image::set_height(int _height) {
  Image::_height = _height;
}

int Image::get_width() const {
  return _width;
}

void Image::set_width(int _width) {
  Image::_width = _width;
}

const string &Image::get_file_name() const {
  return _file_name;
}

void Image::set_file_name(const string &_file_name) {
  Image::_file_name = _file_name;
}

Image::Image(int _height, int _width, const string &_file_name) : _height(_height), _width(_width),
                                                                  _file_name(_file_name) {}

Image Image::read_from_file(const string &_file_name) {
  // TODO: read actual properties
  return Image(256, 256, _file_name);
}

Image::Image() {}
