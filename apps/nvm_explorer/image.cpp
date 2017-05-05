#include "image.h"

int image::get_height() const {
    return _height;
}

void image::set_height(int _height) {
    image::_height = _height;
}

int image::get_width() const {
    return _width;
}

void image::set_width(int _width) {
    image::_width = _width;
}

const string &image::get_file_name() const {
    return _file_name;
}

void image::set_file_name(const string &_file_name) {
    image::_file_name = _file_name;
}

image::image(int _height, int _width, const string &_file_name) : _height(_height), _width(_width),
                                                                  _file_name(_file_name) {}

image image::read_from_file(const string &_file_name) {
    // TODO: read actual properties
    return image(256, 256, _file_name);
}
