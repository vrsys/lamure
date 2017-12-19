// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_MEM_ARRAY_H_
#define PRE_MEM_ARRAY_H_

#include <lamure/pre/array_abstract.h>
#include <lamure/pre/surfel.h>
#include <lamure/pre/prov.h>

namespace lamure
{
namespace pre
{

template<typename T>
class PREPROCESSING_DLL mem_array: public array_abstract<T>
{
public:

    explicit mem_array()
        : array_abstract<T>()
    { reset(); }

    explicit mem_array(const mem_array &other,
                              const size_t offset,
                              const size_t length)
        : array_abstract<T>()
    { reset(other.mem_data_, offset, length); }

    explicit mem_array(const std::shared_ptr<std::vector<T>> &mem_data,
                              const size_t offset,
                              const size_t length)
        : array_abstract<T>()
    { reset(mem_data, offset, length); }

    T read_surfel(const size_t index) const override;
    T const &read_surfel_ref(const size_t index) const;
    void write_surfel(const T &surfel, const size_t index) const override;

    std::shared_ptr<std::vector<T>> & mem_data() { return mem_data_; }
    const std::shared_ptr<std::vector<T>> & mem_data() const { return mem_data_; }

    void reset() override;
    void reset(const std::shared_ptr<std::vector<T>> &mem_data,
               const size_t offset,
               const size_t length);

protected:

    std::shared_ptr<std::vector<T>> mem_data_;

};

template<typename T>
struct array_traits<mem_array<T>>
{
    static const bool is_out_of_core = false;
    static const bool is_in_core = true;
};

typedef mem_array<surfel> surfel_mem_array;
typedef mem_array<surfel> prov_mem_array;

} // namespace pre
} // namespace lamure

#include <lamure/pre/mem_array.inl>

#endif // PRE_MEM_ARRAY_H_
