// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_FORMAT_XYZ_H_
#define PRE_FORMAT_XYZ_H_

#include <lamure/pre/platform.h>
#include <lamure/pre/io/format_abstract.h>

namespace lamure {
namespace pre {

class PREPROCESSING_DLL FormatXYZ : public FormatAbstract
{
public:
    explicit            FormatXYZ()
                            : FormatAbstract()
                            { has_normals_ = false;
                              has_radii_   = false;
                              has_color_   = true; }

protected:
    virtual void        Read(const std::string& filename, SurfelCallbackFuntion callback);
    virtual void        Write(const std::string& filename, BufferCallbackFuntion callback);

};

} // namespace pre
} // namespace lamure

#endif // PRE_FORMAT_XYZ_H_

