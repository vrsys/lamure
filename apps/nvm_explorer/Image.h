#ifndef LAMURE_IMAGE_H
#define LAMURE_IMAGE_H

#include "exif.h"
#include <scm/core.h>
#include <scm/core/math.h>
#include <scm/core/math/vec.h>
#include <scm/core/math/vec2.h>
#include <scm/gl_util/primitives/quad.h>
#include <string>

using namespace std;

class Image
{
  private:
    int _height;
    int _width;
    string _file_name;
    double _focal_length;
    double _fp_resolution_x;
    double _fp_resolution_y;

  public:
    scm::shared_ptr<scm::gl::quad_geometry> _quad;

    Image();

    Image(int _height, int _width, const string &_file_name, double _focal_length, double _fp_resolution_x, double _fp_resolution_y);

    int get_height() const;

    void set_height(int _height);

    int get_width() const;

    void set_width(int _width);

    const string &get_file_name() const;

    void set_file_name(const string &_file_name);

    double get_focal_length() const;

    void set_focal_length(double _focal_length);

    double get_fp_resolution_x() const;

    void set_fp_resolution_x(double _fp_resolution_x);

    double get_fp_resolution_y() const;

    void set_fp_resolution_y(double _fp_resolution_y);

    static Image read_from_file(const string &_file_name);
};

#endif // LAMURE_IMAGE_H
