// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_FORMAT_ABSTRACT_H_
#define PRE_FORMAT_ABSTRACT_H_

#include <lamure/pre/platform.h>
#include <lamure/pre/io/file.h>
#include <lamure/pre/logger.h>
#include <map>

namespace lamure {
namespace pre {

class converter;

class PREPROCESSING_DLL format_abstract
{
public:

    friend class converter;
    typedef std::function<void(const surfel&)> surfel_callback_funtion;
    typedef std::function<bool(surfel_vector&)> buffer_callback_function;

    explicit            format_abstract()
                            : has_normals_(false),
                              has_radii_(false),
                              has_color_(false) {}

    virtual             ~format_abstract() {}

    const bool          has_normals() const { return has_normals_; }
    const bool          has_radii() const { return has_radii_; }
    const bool          has_color() const { return has_color_; }

protected:

    virtual void        read(const std::string& filename, surfel_callback_funtion callback) = 0;
    virtual void        write(const std::string& filename, buffer_callback_function callback) = 0;
    
    bool                has_normals_;
    bool                has_radii_;
    bool                has_color_;

};


template<typename T> 
format_abstract* create_format_instance() { return new T; }

typedef std::map<std::string, format_abstract*(*)()> format_factory;

} // namespace pre
} // namespace lamure

#endif // PRE_FORMAT_ABSTRACT_H_
