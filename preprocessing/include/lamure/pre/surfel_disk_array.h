// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_SURFEL_DISK_ARRAY_H_
#define PRE_SURFEL_DISK_ARRAY_H_

#include <lamure/pre/surfel_array_abstract.h>
#include <lamure/pre/io/file.h>

namespace lamure
{
namespace pre
{

class PREPROCESSING_DLL surfel_disk_array: public surfel_array_abstract
{
public:

    explicit surfel_disk_array()
        : surfel_array_abstract()
    { reset(); }

    explicit surfel_disk_array(const surfel_disk_array &other,
                               const size_t offset,
                               const size_t length)
        : surfel_array_abstract()
    { reset(other.file_, offset, length); }

    explicit surfel_disk_array(const shared_file &file,
                               const size_t offset,
                               const size_t length)
        : surfel_array_abstract()
    { reset(file, offset, length); }

    surfel read_surfel(const size_t index) const override;
    void write_surfel(const surfel &surfel, const size_t index) const override;

    shared_file &file()
    { return file_; }
    const shared_file &file() const
    { return file_; }

    void reset() override;
    void reset(const shared_file &file,
               const size_t offset,
               const size_t length);

    shared_surfel_vector
    read_all() const;

    void write_all(const shared_surfel_vector &data,
                   const size_t offset_in_vector);

private:

    shared_file file_;

};

template<>
struct surfel_array_traits<surfel_disk_array>
{
    static const bool is_out_of_core = true;
    static const bool is_in_core = false;
};

} // namespace pre
} // namespace lamure

#endif // PRE_SURFEL_DISK_ARRAY_H_
