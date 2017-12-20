// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/surfel_mem_array.h>

namespace lamure
{
namespace pre
{

surfel surfel_mem_array::
read_surfel(const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);
    assert(offset_ + index < surfel_mem_data_->size());

    return surfel_mem_data_->operator[](offset_ + index);
}

surfel const &surfel_mem_array::
read_surfel_ref(const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);
    assert(offset_ + index < surfel_mem_data_->size());

    return surfel_mem_data_->operator[](offset_ + index);
}

void surfel_mem_array::
write_surfel(const surfel &surfel, const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);
    assert(offset_ + index < surfel_mem_data_->size());

    surfel_mem_data_->at(offset_ + index) = surfel;
}


prov surfel_mem_array::
read_prov(const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);
    assert(offset_ + index < prov_mem_data_->size());

    return prov_mem_data_->operator[](offset_ + index);
}

prov const &surfel_mem_array::
read_prov_ref(const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);
    assert(offset_ + index < prov_mem_data_->size());

    return prov_mem_data_->operator[](offset_ + index);
}

void surfel_mem_array::
write_prov(const prov &prov, const size_t index) const
{
    assert(!is_empty_);
    assert(index < length_);
    assert(offset_ + index < prov_mem_data_->size());

    prov_mem_data_->at(offset_ + index) = prov;
}

void surfel_mem_array::
reset()
{
    array_abstract<surfel>::reset();
    surfel_mem_data_.reset();
    prov_mem_data_.reset();
}

void surfel_mem_array::
reset(const std::shared_ptr<std::vector<surfel>> &surfel_mem_data,
      const size_t offset,
      const size_t length)
{
    is_empty_ = false;
    offset_ = offset;
    length_ = length;
    surfel_mem_data_ = surfel_mem_data;
    prov_mem_data_.reset();
}

void surfel_mem_array::
reset(const std::shared_ptr<std::vector<surfel>> &surfel_mem_data,
      const std::shared_ptr<std::vector<prov>> &prov_mem_data,
      const size_t offset,
      const size_t length)
{
    is_empty_ = false;
    offset_ = offset;
    length_ = length;
    surfel_mem_data_ = surfel_mem_data;
    prov_mem_data_ = prov_mem_data;
}

} // namespace pre
} // namespace lamure
