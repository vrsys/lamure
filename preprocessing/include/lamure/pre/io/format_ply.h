// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_FORMAT_PLY_H_
#define PRE_FORMAT_PLY_H_

#include <functional>

#include <lamure/pre/platform.h>
#include <lamure/pre/io/format_abstract.h>

namespace lamure {
namespace pre {

class PREPROCESSING_DLL FormatPLY : public FormatAbstract
{
public:
    explicit            FormatPLY()
                            : FormatAbstract()
                            { has_normals_ = false;
                              has_radii_   = false;
                              has_color_   = true; }


protected:
    virtual void        Read(const std::string& filename, SurfelCallbackFuntion callback);
    virtual void        Write(const std::string& filename, BufferCallbackFuntion callback);

private:
    Surfel              current_surfel_;

    template <typename ScalarType> 
    std::function <void (ScalarType)> scalar_callback(const std::string& element_name, 
                                                      const std::string& property_name);
};

template <>
std::function<void(float)> FormatPLY::
scalar_callback(const std::string& element_name, const std::string& property_name);

template <>
std::function<void(uint8_t)> FormatPLY::
scalar_callback(const std::string& element_name, const std::string& property_name);

} // namespace pre
} // namespace lamure

#endif // PRE_FORMAT_PLY_H_

