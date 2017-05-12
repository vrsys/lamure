#include "Image.h"

int Image::get_height () const
{
    return _height;
}

void Image::set_height (int _height)
{
    Image::_height = _height;
}

int Image::get_width () const
{
    return _width;
}

void Image::set_width (int _width)
{
    Image::_width = _width;
}

const string &Image::get_file_name () const
{
    return _file_name;
}

void Image::set_file_name (const string &_file_name)
{
    Image::_file_name = _file_name;
}

Image Image::read_from_file (const string &_file_name)
{
    FILE *fp = fopen (_file_name.c_str (), "rb");
    if (!fp)
        {
            printf ("Can't open file: %s\n", _file_name.c_str ());
            return Image ();
        }
    fseek (fp, 0, SEEK_END);
    size_t fsize = (size_t) ftell (fp);
    rewind (fp);
    unsigned char *buf = new unsigned char[fsize];
    if (fread (buf, 1, fsize, fp) != fsize)
        {
            printf ("Can't read file: %s\n", _file_name.c_str ());
            delete[] buf;
            return Image ();
        }
    fclose (fp);

    easyexif::EXIFInfo result;
    result.parseFrom (buf, (unsigned int) fsize);

    double focal_length_m = result.FocalLength * 0.001;
    double resolution_x = result.LensInfo.FocalPlaneResolutionUnit == 2 ? result.LensInfo.FocalPlaneXResolution / 0.0254: result.LensInfo.FocalPlaneXResolution / 0.01;
    double resolution_y = result.LensInfo.FocalPlaneResolutionUnit == 2 ? result.LensInfo.FocalPlaneYResolution / 0.0254: result.LensInfo.FocalPlaneYResolution / 0.01;

    printf ("Focal length: %f, FP Resolution X: %f, Y: %f\n",
            focal_length_m,
            resolution_x,
            resolution_y);

    return Image (result.ImageHeight,
                  result.ImageWidth,
                  _file_name,
                  focal_length_m,
                  resolution_x,
                  resolution_y
                  );
}

Image::Image ()
{}

Image::Image (int _height,
              int _width,
              const string &_file_name,
              double _focal_length,
              double _fp_resolution_x,
              double _fp_resolution_y)
    : _height (_height),
      _width (_width),
      _file_name (_file_name),
      _focal_length (_focal_length),
      _fp_resolution_x (_fp_resolution_x),
      _fp_resolution_y (_fp_resolution_y)
{}

double Image::get_focal_length () const
{
    return _focal_length;
}

void Image::set_focal_length (double _focal_length)
{
    Image::_focal_length = _focal_length;
}

double Image::get_fp_resolution_x () const
{
    return _fp_resolution_x;
}

void Image::set_fp_resolution_x (double _fp_resolution_x)
{
    Image::_fp_resolution_x = _fp_resolution_x;
}

double Image::get_fp_resolution_y () const
{
    return _fp_resolution_y;
}

void Image::set_fp_resolution_y (double _fp_resolution_y)
{
    Image::_fp_resolution_y = _fp_resolution_y;
}