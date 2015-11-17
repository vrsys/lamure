// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/surfel_mem_array.h>

namespace lamure {
namespace pre {

Surfel SurfelMemArray::
ReadSurfel(const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);
    assert(offset_ + index < mem_data_->size());

    return mem_data_->at(offset_ + index);
}

void SurfelMemArray::
WriteSurfel(const Surfel& surfel, const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);
    assert(offset_ + index < mem_data_->size());

    mem_data_->at(offset_ + index) = surfel;
}

void SurfelMemArray::
Reset()
{
    SurfelArrayAbstract::Reset();
    mem_data_.reset();
}

void SurfelMemArray::
Reset(const SharedSurfelVector& mem_data,
      const size_t offset,
      const size_t length)
{
    is_empty_ = false;
    offset_ = offset;
    length_ = length;
    mem_data_ = mem_data;
}

} // namespace pre
} // namespace lamure
