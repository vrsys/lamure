// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_FORMAT_BIN_H_
#define PRE_FORMAT_BIN_H_

#include <lamure/platform_xyz.h>
#include <lamure/xyz/io/file.h>
#include <lamure/xyz/io/format_abstract.h>

namespace lamure {
namespace xyz {

class XYZ_DLL format_bin : public format_abstract
{
public:
    explicit            format_bin()
                            : format_abstract()
                            { has_normals_ = true;
                              has_radii_   = true;
                              has_color_   = true; }

protected:
    virtual void        read(const std::string& filename, surfel_callback_funtion callback);
    virtual void        write(const std::string& filename, buffer_callback_function callback);

};

} // namespace xyz
} // namespace lamure

#endif // PRE_FORMAT_BIN_H_
