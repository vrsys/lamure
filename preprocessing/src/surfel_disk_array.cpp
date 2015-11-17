// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/surfel_disk_array.h>
#include <lamure/pre/logger.h>

namespace lamure {
namespace pre {

Surfel SurfelDiskArray::
ReadSurfel(const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);

    return file_->Read(offset_ + index);
}

void SurfelDiskArray::
WriteSurfel(const Surfel& surfel, const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);

    file_->Write(surfel, offset_ + index);
}

void SurfelDiskArray::
Reset()
{
    SurfelArrayAbstract::Reset();
    file_.reset();
}

void SurfelDiskArray::
Reset(const SharedFile& file,
      const size_t offset,
      const size_t length)
{
    is_empty_ = false;
    offset_ = offset;
    length_ = length;
    file_ = file;
}

SharedSurfelVector SurfelDiskArray::
ReadAll() const
{
    if (is_empty()) {
        LOGGER_ERROR("Attempt to read from an empty SurfelDiskArray");
        exit(1);
    }

    SurfelVector data(length_);
    file_->Read(&data, 0, offset_, length_);
    return std::make_shared<SurfelVector>(data);
}

void SurfelDiskArray::
WriteAll(const SharedSurfelVector& data,
         const size_t offset_in_vector)
{
    if (is_empty()) {
        LOGGER_ERROR("Attempt to write to an empty SurfelDiskArray");
        exit(1);
    }

    file_->Write(data.get(), offset_in_vector, offset_, length_);
}

} // namespace pre
} // namespace lamure
