// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_FORMAT_XYZ_PROV_H_
#define PRE_FORMAT_XYZ_PROV_H_

#include <lamure/pre/platform.h>

#include <lamure/types.h>

namespace lamure
{
namespace pre
{

class PREPROCESSING_DLL format_xyz_prov
{
public:
    static void convert(const std::string& in_file, const std::string& out_file, bool xyz_rgb);

protected:

};

} // namespace pre
} // namespace lamure

#endif // PRE_FORMAT_XYZ_PROV_H_

