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

namespace lamure {
namespace pre {

class PREPROCESSING_DLL SurfelDiskArray : public SurfelArrayAbstract
{
public:

    explicit            SurfelDiskArray()
                            : SurfelArrayAbstract() { Reset(); }

    explicit            SurfelDiskArray(const SurfelDiskArray& other,
                                        const size_t offset,
                                        const size_t length)
                            : SurfelArrayAbstract() { Reset(other.file_, offset, length); }

    explicit            SurfelDiskArray(const SharedFile& file,
                                        const size_t offset,
                                        const size_t length)
                            : SurfelArrayAbstract() { Reset(file, offset, length); }

    Surfel              ReadSurfel(const size_t index) const override;
    void                WriteSurfel(const Surfel& surfel, const size_t index) const override;

    SharedFile&         file() { return file_; }
    const SharedFile&   file() const { return file_; }

    void                Reset() override;
    void                Reset(const SharedFile &file,
                              const size_t offset,
                              const size_t length);

    SharedSurfelVector
                        ReadAll() const;

    void                WriteAll(const SharedSurfelVector& data,
                                 const size_t offset_in_vector);

private:

    SharedFile          file_;

};

template <>
struct SurfelArrayTraits<SurfelDiskArray>
{
    static const bool IsOOC = true;
    static const bool IsIC = false;
};

} // namespace pre
} // namespace lamure

#endif // PRE_SURFEL_DISK_ARRAY_H_
