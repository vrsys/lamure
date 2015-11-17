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

class Converter;

class PREPROCESSING_DLL FormatAbstract
{
public:

    friend class Converter;
    typedef std::function<void(const Surfel&)> SurfelCallbackFuntion;
    typedef std::function<bool(SurfelVector&)> BufferCallbackFuntion;

    explicit            FormatAbstract()
                            : has_normals_(false),
                              has_radii_(false),
                              has_color_(false) {}

    virtual             ~FormatAbstract() {}

    const bool          has_normals() const { return has_normals_; }
    const bool          has_radii() const { return has_radii_; }
    const bool          has_color() const { return has_color_; }

protected:

    virtual void        Read(const std::string& filename, SurfelCallbackFuntion callback) = 0;
    virtual void        Write(const std::string& filename, BufferCallbackFuntion callback) = 0;
    
    bool                has_normals_;
    bool                has_radii_;
    bool                has_color_;

};


template<typename T> 
FormatAbstract* CreateFormatInstance() { return new T; }

typedef std::map<std::string, FormatAbstract*(*)()> FormatFactory;

} // namespace pre
} // namespace lamure

#endif // PRE_FORMAT_ABSTRACT_H_
