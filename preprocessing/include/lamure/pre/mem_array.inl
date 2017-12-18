// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

//#include <lamure/pre/surfel_mem_array.h>

namespace lamure
{
namespace pre
{

template<typename T>
T mem_array<T>::
read_surfel(const size_t index) const
{
    assert(!is_empty_);
    assert(index < array_abstract<T>::length_);
    assert(array_abstract<T>::offset_ + index < mem_data_->size());

    return mem_data_->operator[](array_abstract<T>::offset_ + index);
}

template<typename T>
T const &mem_array<T>::
read_surfel_ref(const size_t index) const
{
    assert(!is_empty_);
    assert(index < array_abstract<T>::length_);
    assert(array_abstract<T>::offset_ + index < mem_data_->size());

    return mem_data_->operator[](array_abstract<T>::offset_ + index);
}

template<typename T>
void mem_array<T>::
write_surfel(const T &surfel, const size_t index) const
{
    assert(!is_empty_);
    assert(index < array_abstract<T>::length_);
    assert(array_abstract<T>::offset_ + index < mem_data_->size());

    mem_data_->at(array_abstract<T>::offset_ + index) = surfel;
}

template<typename T>
void mem_array<T>::
reset()
{
    array_abstract<T>::reset();
    mem_data_.reset();
}

template<typename T>
void mem_array<T>::
reset(const std::shared_ptr<std::vector<T>> &mem_data,
      const size_t offset,
      const size_t length)
{
    array_abstract<T>::is_empty_ = false;
    array_abstract<T>::offset_ = offset;
    array_abstract<T>::length_ = length;
    mem_data_ = mem_data;
}

} // namespace pre
} // namespace lamure
