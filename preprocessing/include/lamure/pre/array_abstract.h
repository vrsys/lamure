// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_ARRAY_ABSTRACT_H_
#define PRE_ARRAY_ABSTRACT_H_

#include <lamure/pre/platform.h>
#include <lamure/types.h>

#include <memory>
#include <vector>

namespace lamure
{
namespace pre
{

template<typename T>
class PREPROCESSING_DLL array_abstract
{
public:

    explicit array_abstract()
        : is_empty_(true),
          offset_(0),
          length_(0)
    {}

    virtual             ~array_abstract();

    const bool is_empty() const { return is_empty_; }
    const size_t offset() const { return offset_; }
    const size_t length() const { return length_; }

    void set_offset(const size_t value) { offset_ = value; }
    void set_length(const size_t value) { length_ = value; }

    virtual T read_surfel(const size_t index) const = 0;
    virtual void write_surfel(const T &surfel, const size_t index) const = 0;
    virtual void reset()
    {
        is_empty_ = true;
        offset_ = 0;
        length_ = 0;
    };

protected:

    bool is_empty_;
    size_t offset_;
    size_t length_;
};

template<typename T>
struct array_traits;

} // namespace pre
} // namespace lamure

#include <lamure/pre/array_abstract.inl>

#endif // PRE_ARRAY_ABSTRACT_H_
