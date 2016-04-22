// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_SURFEL_MEM_ARRAY_H_
#define PRE_SURFEL_MEM_ARRAY_H_

#include <lamure/xyz/surfel_array_abstract.h>

namespace lamure {
namespace xyz {

class XYZ_DLL surfel_mem_array : public surfel_array_abstract
{
public:

    explicit            surfel_mem_array()
                            : surfel_array_abstract() { reset(); }

    explicit            surfel_mem_array(const surfel_mem_array& other,
                                       const size_t offset,
                                       const size_t length)
                            : surfel_array_abstract() { reset(other.mem_data_, offset, length); }

    explicit            surfel_mem_array(const shared_surfel_vector& mem_data,
                                       const size_t offset,
                                       const size_t length)
                            : surfel_array_abstract() { reset(mem_data, offset, length); }

    surfel              read_surfel(const size_t index) const override;
    surfel const&       read_surfel_ref(const size_t index) const;
    void                write_surfel(const surfel& surfel, const size_t index) const override;

    shared_surfel_vector&
                        mem_data() { return mem_data_; }
    const shared_surfel_vector&
                        mem_data() const { return mem_data_; }

    void                reset() override;
    void                reset(const shared_surfel_vector& mem_data,
                              const size_t offset,
                              const size_t length);

private:

    shared_surfel_vector mem_data_;

};

template <>
struct surfel_array_traits<surfel_mem_array>
{
    static const bool is_out_of_core = false;
    static const bool is_in_core = true;
};

} // namespace xyz
} // namespace lamure

#endif // PRE_SURFEL_MEM_ARRAY_H_
