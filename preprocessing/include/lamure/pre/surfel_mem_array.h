// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_SURFEL_MEM_ARRAY_H_
#define PRE_SURFEL_MEM_ARRAY_H_

#include <lamure/pre/surfel_array_abstract.h>

namespace lamure {
namespace pre {

class PREPROCESSING_DLL SurfelMemArray : public SurfelArrayAbstract
{
public:

    explicit            SurfelMemArray()
                            : SurfelArrayAbstract() { Reset(); }

    explicit            SurfelMemArray(const SurfelMemArray& other,
                                       const size_t offset,
                                       const size_t length)
                            : SurfelArrayAbstract() { Reset(other.mem_data_, offset, length); }

    explicit            SurfelMemArray(const SharedSurfelVector& mem_data,
                                       const size_t offset,
                                       const size_t length)
                            : SurfelArrayAbstract() { Reset(mem_data, offset, length); }

    Surfel              ReadSurfel(const size_t index) const override;
    void                WriteSurfel(const Surfel& surfel, const size_t index) const override;

    SharedSurfelVector&
                        mem_data() { return mem_data_; }
    const SharedSurfelVector&
                        mem_data() const { return mem_data_; }

    void                Reset() override;
    void                Reset(const SharedSurfelVector& mem_data,
                              const size_t offset,
                              const size_t length);

private:

    SharedSurfelVector mem_data_;

};

template <>
struct SurfelArrayTraits<SurfelMemArray>
{
    static const bool IsOOC = false;
    static const bool IsIC = true;
};

} // namespace pre
} // namespace lamure

#endif // PRE_SURFEL_MEM_ARRAY_H_
