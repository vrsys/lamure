// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_PRE_FORMAT_XYZ_ALL_H_
#define PRE_PRE_FORMAT_XYZ_ALL_H_

#include <lamure/platform_xyz.h>
#include <lamure/xyz/io/format_abstract.h>

namespace lamure {
namespace xyz {

class XYZ_DLL format_xyzall : public format_abstract
{
public:
    explicit            format_xyzall()
                            : format_abstract()
                            { has_normals_ = true;
                              has_radii_   = true;
                              has_color_   = true; }

protected:
    virtual void        read(const std::string& filename, surfel_callback_funtion callback) override;
    virtual void        write(const std::string& filename, buffer_callback_function callback) override;

};

} // namespace xyz
} // namespace lamure

#endif // PRE_FORMAT_XYZ_ALL_H_
