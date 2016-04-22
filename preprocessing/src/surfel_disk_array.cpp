// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/surfel_disk_array.h>
#include <lamure/logger.h>
#include <lamure/assert.h>

namespace lamure {
namespace pre {

surfel surfel_disk_array::
read_surfel(const size_t index) const
{
    ASSERT(!is_empty_);
    ASSERT(index < length_);

    return file_->read(offset_ + index);
}

void surfel_disk_array::
write_surfel(const surfel& surfel, const size_t index) const
{
    ASSERT(!is_empty_);
    ASSERT(index < length_);

    file_->write(surfel, offset_ + index);
}

void surfel_disk_array::
reset()
{
    surfel_array_abstract::reset();
    file_.reset();
}

void surfel_disk_array::
reset(const shared_file& file,
      const size_t offset,
      const size_t length)
{
    is_empty_ = false;
    offset_ = offset;
    length_ = length;
    file_ = file;
}

shared_surfel_vector surfel_disk_array::
read_all() const
{
    if (is_empty()) {
        LAMURE_LOG_ERROR("Attempt to read from an empty surfel_disk_array");
        exit(1);
    }

    surfel_vector data(length_);
    file_->read(&data, 0, offset_, length_);
    return std::make_shared<surfel_vector>(data);
}

void surfel_disk_array::
write_all(const shared_surfel_vector& data,
         const size_t offset_in_vector)
{
    if (is_empty()) {
        LAMURE_LOG_ERROR("Attempt to write to an empty surfel_disk_array");
        exit(1);
    }

    file_->write(data.get(), offset_in_vector, offset_, length_);
}

} // namespace pre
} // namespace lamure
