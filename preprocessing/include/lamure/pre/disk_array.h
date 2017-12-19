// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_DISK_ARRAY_H_
#define PRE_DISK_ARRAY_H_

#include <lamure/pre/array_abstract.h>
#include <lamure/pre/io/file.h>
#include <lamure/pre/surfel.h>
#include <lamure/pre/prov.h>

namespace lamure
{
namespace pre
{

template<typename T>
class PREPROCESSING_DLL disk_array: public array_abstract<T>
{
public:

    typedef std::shared_ptr<file<T>> shared_file;

    explicit disk_array()
        : array_abstract<T>() { 
      reset(); 
    }

    explicit disk_array(const disk_array &other,
                               const size_t offset,
                               const size_t length)
        : array_abstract<T>() { 
      reset(other.file_, offset, length); 
    }

    explicit disk_array(const std::shared_ptr<file<T>> &file,
                               const size_t offset,
                               const size_t length)
        : array_abstract<T>() { 
      reset(file, offset, length); 
    }

    T read_surfel(const size_t index) const override;
    void write_surfel(const T &surfel, const size_t index) const override;

    shared_file &get_file() { return file_; }
    const shared_file &get_file() const { return file_; }

    void reset() override;
    void reset(const shared_file &file,
               const size_t offset,
               const size_t length);

    std::shared_ptr<std::vector<T>>
    read_all() const;

    void write_all(const std::shared_ptr<std::vector<T>> &data,
                   const size_t offset_in_vector);

private:

    shared_file file_;

};

template<typename T>
struct array_traits<disk_array<T>>
{
    static const bool is_out_of_core = true;
    static const bool is_in_core = false;
};

typedef disk_array<surfel> surfel_disk_array;
typedef disk_array<prov> prov_disk_array;

} // namespace pre
} // namespace lamure

#include <lamure/pre/disk_array.inl>


#endif // PRE_DISK_ARRAY_H_
