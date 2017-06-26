#include "Image.h"
#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/data/imaging/texture_loader_dds.h>

int Image::get_height() const { return _height; }

void Image::set_height(int _height) { Image::_height = _height; }

int Image::get_width() const { return _width; }

void Image::set_width(int _width) { Image::_width = _width; }

scm::gl::sampler_state_ptr Image::get_state() { return _state; }

void Image::load_texture(scm::shared_ptr<scm::gl::render_device> device)
{
    scm::gl::texture_loader tl;
    std::cout << "creating texture" << std::endl;
    _texture.reset();
    _texture = tl.load_texture_2d(*device, _file_name, true, false);

    _state = device->create_sampler_state(scm::gl::FILTER_MIN_MAG_LINEAR, scm::gl::WRAP_CLAMP_TO_EDGE);
}

scm::gl::texture_2d_ptr Image::get_texture() { return _texture; }

void Image::update_transformation(scm::math::mat4f transformation, float scale_frustum)
{
    scm::math::mat4f matrix_translation = scm::math::make_translation(scm::math::vec3f(0.0, 0.0, -_focal_length * scale_frustum));
    // scm::math::mat4f matrix_rotation = scm::math::mat4f(_orientation.to_matrix());

    // scm::math::translate(transformation, scm::math::vec3f(0.0, 0.0, -1.0));
    // normalize quad to 1m*1m
    // float scale = 0.5;
    // // get the width of quad in world dimension (should be sensor size)
    // scale *= (1.0f/_still_image.get_fp_resolution_x()) * _still_image.get_width();
    // // increase size for better visibility
    // scale *= 10.0f;

    // float aspect_ratio = float(_still_image.get_height())/_still_image.get_width();
    // scm::math::mat4f matrix_scale = scm::math::make_scale(scm::math::vec3f(scale, scale * aspect_ratio, scale));

    // scm::math::translate(transformation, scm::math::vec3f(0.0, 0.0, 0.0));
    // normalize quad to 1m*1m
    float scale = 0.5;
    // get the width of quad in world dimension (should be sensor size)
    scale *= (1.0f / _fp_resolution_x) * _width;
    // increase size for better visibility
    scale *= scale_frustum;

    float aspect_ratio = float(_height) / _width;
    scm::math::mat4f matrix_scale = scm::math::make_scale(scm::math::vec3f(scale, scale * aspect_ratio, scale));
    _transformation = transformation * matrix_translation * matrix_scale;
    // // _transformation = matrix_translation;
    // // _transformation =  matrix_rotation;
    // // _transformation =  scm::math::inverse(matrix_rotation) * matrix_translation;
    // // _transformation = matrix_translation * matrix_rotation;
    // _transformation = matrix_translation * matrix_rotation * matrix_scale;
}

float Image::get_width_world() { return (1.0f / _fp_resolution_x) * _width; }

float Image::get_height_world() { return (1.0f / _fp_resolution_y) * _height; }

const scm::math::mat4f &Image::get_transformation() const { return _transformation; }

const string &Image::get_file_name() const { return _file_name; }

void Image::set_file_name(const string &_file_name) { Image::_file_name = _file_name; }

Image Image::read_from_file(const string &_file_name)
{
    FILE *fp = fopen(_file_name.c_str(), "rb");
    if(!fp)
    {
        printf("Can't open file: %s\n", _file_name.c_str());
        return Image();
    }
    fseek(fp, 0, SEEK_END);
    size_t fsize = (size_t)ftell(fp);
    rewind(fp);
    unsigned char *buf = new unsigned char[fsize];
    if(fread(buf, 1, fsize, fp) != fsize)
    {
        printf("Can't read file: %s\n", _file_name.c_str());
        delete[] buf;
        return Image();
    }
    fclose(fp);

    easyexif::EXIFInfo result;
    result.parseFrom(buf, (unsigned int)fsize);

    double focal_length_m = result.FocalLength * 0.001;
    double resolution_x = result.LensInfo.FocalPlaneResolutionUnit == 2 ? result.LensInfo.FocalPlaneXResolution / 0.0254 : result.LensInfo.FocalPlaneXResolution / 0.01;
    double resolution_y = result.LensInfo.FocalPlaneResolutionUnit == 2 ? result.LensInfo.FocalPlaneYResolution / 0.0254 : result.LensInfo.FocalPlaneYResolution / 0.01;

    printf("Focal length: %f, FP Resolution X: %f, Y: %f\n", focal_length_m, resolution_x, resolution_y);

    return Image(result.ImageHeight, result.ImageWidth, _file_name, focal_length_m, resolution_x, resolution_y);
}

Image::Image() {}

Image::Image(int _height, int _width, const string &_file_name, double _focal_length, double _fp_resolution_x, double _fp_resolution_y)
    : _height(_height), _width(_width), _file_name(_file_name), _focal_length(_focal_length), _fp_resolution_x(_fp_resolution_x), _fp_resolution_y(_fp_resolution_y)
{
}

double Image::get_focal_length() const { return _focal_length; }

void Image::set_focal_length(double _focal_length) { Image::_focal_length = _focal_length; }

double Image::get_fp_resolution_x() const { return _fp_resolution_x; }

void Image::set_fp_resolution_x(double _fp_resolution_x) { Image::_fp_resolution_x = _fp_resolution_x; }

double Image::get_fp_resolution_y() const { return _fp_resolution_y; }

void Image::set_fp_resolution_y(double _fp_resolution_y) { Image::_fp_resolution_y = _fp_resolution_y; }
