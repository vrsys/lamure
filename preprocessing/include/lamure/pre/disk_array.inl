// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

//#include <lamure/pre/surfel_disk_array.h>
#include <lamure/pre/logger.h>

namespace lamure
{
namespace pre
{

template<typename T>
T disk_array<T>::
read_surfel(const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);

    return file_->read(array_abstract<T>::offset_ + index);
}

template<typename T>
void disk_array<T>::
write_surfel(const T &surfel, const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);

    file_->write(surfel, array_abstract<T>::offset_ + index);
}

template<typename T>
void disk_array<T>::
reset()
{
    array_abstract<T>::reset();
    file_.reset();
}

template<typename T>
void disk_array<T>::
reset(const std::shared_ptr<file<T>> &file,
      const size_t offset,
      const size_t length)
{
    array_abstract<T>::is_empty_ = false;
    array_abstract<T>::offset_ = offset;
    array_abstract<T>::length_ = length;
    file_ = file;
}

template<typename T>
std::shared_ptr<std::vector<T>> disk_array<T>::
read_all() const
{
    if (array_abstract<T>::is_empty()) {
        LOGGER_ERROR("Attempt to read from an empty disk_array");
        exit(1);
    }

    std::vector<T> data(array_abstract<T>::length_);
    file_->read(&data, 0, array_abstract<T>::offset_, array_abstract<T>::length_);
    return std::make_shared<std::vector<T>>(data);
}

template<typename T>
void disk_array<T>::
write_all(const std::shared_ptr<std::vector<T>> &data,
          const size_t offset_in_vector)
{
    if (array_abstract<T>::is_empty()) {
        LOGGER_ERROR("Attempt to write to an empty disk_array");
        exit(1);
    }

    file_->write(data.get(), offset_in_vector, array_abstract<T>::offset_, array_abstract<T>::length_);
}

} // namespace pre
} // namespace lamure
