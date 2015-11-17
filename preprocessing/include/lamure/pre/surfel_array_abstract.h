// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_SURFEL_ARRAY_ABSTRACT_H_
#define PRE_SURFEL_ARRAY_ABSTRACT_H_

#include <lamure/pre/surfel.h>

namespace lamure
{
namespace pre
{

class PREPROCESSING_DLL SurfelArrayAbstract
{
public:

    explicit           SurfelArrayAbstract()
                            : is_empty_(true),
                              offset_(0),
                              length_(0) {}

    virtual             ~SurfelArrayAbstract();

    const bool          is_empty() const { return is_empty_; }
    const size_t        offset() const { return offset_; }
    const size_t        length() const { return length_; }

    void                set_offset(const size_t value) { offset_ = value; }
    void                set_length(const size_t value) { length_ = value; }


    virtual Surfel      ReadSurfel(const size_t index) const = 0;
    virtual void        WriteSurfel(const Surfel& surfel, const size_t index) const = 0;
    virtual void        Reset() { is_empty_ = true; offset_ = 0; length_ = 0; };

protected:

    bool                is_empty_;
    size_t              offset_;
    size_t              length_;
};

template <typename T>
struct SurfelArrayTraits;

} // namespace pre
} // namespace lamure

#endif // PRE_SURFEL_ARRAY_ABSTRACT_H_
