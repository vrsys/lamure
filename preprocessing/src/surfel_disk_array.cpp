// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/surfel_disk_array.h>
#include <lamure/pre/logger.h>

namespace lamure
{
namespace pre
{

surfel surfel_disk_array::
read_surfel(const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);

    return surfel_file_->read(offset_ + index);
}

void surfel_disk_array::
write_surfel(const surfel &surfel, const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);

    surfel_file_->write(surfel, offset_ + index);
}


prov surfel_disk_array::
read_prov(const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);

    return prov_file_->read(offset_ + index);
}

void surfel_disk_array::
write_prov(const prov &prov, const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);

    prov_file_->write(prov, offset_ + index);
}

void surfel_disk_array::
reset()
{
    array_abstract<surfel>::reset();
    surfel_file_.reset();
    prov_file_.reset();
    has_provenance_ = false;
}


void surfel_disk_array::
reset(const std::shared_ptr<file<surfel>> &surfel_file,
      const size_t offset,
      const size_t length)
{
    is_empty_ = false;
    offset_ = offset;
    length_ = length;
    surfel_file_ = surfel_file;
    prov_file_.reset();
    has_provenance_ = false;
}


void surfel_disk_array::
reset(const std::shared_ptr<file<surfel>> &surfel_file,
      const std::shared_ptr<file<prov>> &prov_file,
      const size_t offset,
      const size_t length)
{
    is_empty_ = false;
    offset_ = offset;
    length_ = length;
    surfel_file_ = surfel_file;
    prov_file_ = prov_file;
    has_provenance_ = true;
}

std::shared_ptr<std::vector<surfel>> surfel_disk_array::
read_all() const
{
    if (is_empty()) {
        LOGGER_ERROR("Attempt to read from an empty disk_array");
        exit(1);
    }

    std::vector<surfel> data(length_);
    surfel_file_->read(&data, 0, offset_, length_);
    return std::make_shared<std::vector<surfel>>(data);
}

std::shared_ptr<std::vector<prov>> surfel_disk_array::
read_all_prov() const
{
    if (is_empty()) {
        LOGGER_ERROR("Attempt to read from an empty disk_array");
        exit(1);
    }

    std::vector<prov> data(length_);
    prov_file_->read(&data, 0, offset_, length_);
    return std::make_shared<std::vector<prov>>(data);
}


void surfel_disk_array::
write_all(const std::shared_ptr<std::vector<surfel>> &surfel_data,
          const size_t offset_in_vector)
{
    if (is_empty()) {
        LOGGER_ERROR("Attempt to write to an empty disk_array");
        exit(1);
    }

    surfel_file_->write(surfel_data.get(), offset_in_vector, offset_, length_);
    has_provenance_ = false;
}

void surfel_disk_array::
write_all(const std::shared_ptr<std::vector<surfel>> &surfel_data,
          const std::shared_ptr<std::vector<prov>> &prov_data,
          const size_t offset_in_vector)
{
    if (is_empty()) {
        LOGGER_ERROR("Attempt to write to an empty disk_array");
        exit(1);
    }

    surfel_file_->write(surfel_data.get(), offset_in_vector, offset_, length_);
    prov_file_->write(prov_data.get(), offset_in_vector, offset_, length_);

    //std::cout << "write prov to : " << prov_file_->file_name() << std::endl;
    has_provenance_ = true;
}

} // namespace pre
} // namespace lamure
