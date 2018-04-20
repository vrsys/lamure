// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef XYZ_FILE_FORMATS_H_
#define XYZ_FILE_FORMATS_H_

#include <lamure/types.h>

#include <fstream>

#define DEFAULT_PRECISION 15

namespace lamure {
namespace app {

struct xyz {
    real x;
    real y;
    real z;

virtual ~xyz(){}

// ofstream for xyz_rgb
virtual void write_to_file (std::ofstream& out_f) {
    out_f << std::setprecision(DEFAULT_PRECISION) << x << " "
          << std::setprecision(DEFAULT_PRECISION) << y << " "
          << std::setprecision(DEFAULT_PRECISION) << z << " "
          << (unsigned) 255 << " " 
          << (unsigned) 255 << " "
          << (unsigned) 255 << " " << std::endl;
}

virtual void read_from_file(std::ifstream& in_f) {
    in_f >> x;
    in_f >> y;
    in_f >> z;
}

};

struct xyz_rgb : public xyz
{
    unsigned char r;
    unsigned char g;
    unsigned char b;

virtual ~xyz_rgb(){}

// ofstream for xyz_rgb
virtual void write_to_file (std::ofstream& out_f) {
    out_f << std::setprecision(DEFAULT_PRECISION) << x << " "
      << std::setprecision(DEFAULT_PRECISION) << y << " "
      << std::setprecision(DEFAULT_PRECISION) << z << " "
      << (unsigned) r << " "
      << (unsigned) g << " "
      << (unsigned) b << std::endl;
}

virtual void read_from_file(std::ifstream& in_f) {
    in_f >> x;
    in_f >> y;
    in_f >> z;
    unsigned tmp;
    in_f >> tmp;
    r = tmp;
    in_f >> tmp;
    g = tmp;
    in_f >> tmp;
    b = tmp;
}


}; // xyz_rgb

struct xyz_all : public xyz_rgb
{
    real nx;
    real ny;
    real nz;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    real rad;

virtual ~xyz_all(){}

// ofstream for xyz_all
virtual void write_to_file (std::ofstream& out_f) {
    out_f << std::setprecision(DEFAULT_PRECISION) << x << " "
          << std::setprecision(DEFAULT_PRECISION) << y << " "
          << std::setprecision(DEFAULT_PRECISION) << z << " "
          << std::setprecision(DEFAULT_PRECISION) << nx << " "
          << std::setprecision(DEFAULT_PRECISION) << ny << " "
          << std::setprecision(DEFAULT_PRECISION) << nz << " "
          << (unsigned) r << " "
          << (unsigned) g << " "
          << (unsigned) b << " "
          << std::setprecision(DEFAULT_PRECISION) << rad << " " << std::endl;
}

virtual void read_from_file(std::ifstream& in_f) {
    in_f >> x;
    in_f >> y;
    in_f >> z;
    in_f >> nx;
    in_f >> ny;
    in_f >> nz;
    unsigned tmp;
    in_f >> tmp;
    r = tmp;
    in_f >> tmp;
    g = tmp;
    in_f >> tmp;
    b = tmp;
    in_f >> rad;
}
}; // xyz_all



enum class xyz_type {
    //xyz              = 0,
    xyz_rgb          = 1,
    xyz_all          = 2
};


} // app
} // lamure

#endif // XYZ_FILE_FORMATS_H_