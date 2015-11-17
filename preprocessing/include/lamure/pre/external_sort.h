// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_EXTERNAL_SORT_H_
#define PRE_EXTERNAL_SORT_H_

#include <lamure/pre/surfel_disk_array.h>
#include <vector>
#include <lamure/pre/logger.h>

namespace lamure {
namespace pre
{

class PREPROCESSING_DLL ExternalSort
{
public:

    static void         Sort(SurfelDiskArray& array,
                             const size_t memory_limit,
                             const Surfel::CompareFunction& compare);

private:
    explicit            ExternalSort(const size_t memory_limit,
                                     const Surfel::CompareFunction& compare);
                        ExternalSort(const ExternalSort&) = delete;
                        ExternalSort& operator=(const ExternalSort&) = delete;

    class Buffer
    {
        public:
            SurfelDiskArray run;
            SurfelVector data;
            size_t size;
            size_t candidate_pos;
            size_t file_offset;

            Buffer(const SurfelDiskArray& array, const size_t buffer_size)
                : run(array) {
                size = buffer_size;
                candidate_pos = size;
                file_offset = 0;
                data = SurfelVector(size);
            }

            bool Front(Surfel& s) {
                Invalidate();
                if (size > 0) {
                    s = data[candidate_pos];
                    return true;
                } else
                    return false;
            }

            void PopFront() {
                assert(size > 0);
                candidate_pos++;
            }

        private:
            void Invalidate() {
                if (size > 0 && candidate_pos >= size) {
                    if (file_offset >= run.length()) {
                        size = 0;
                        return;
                    }

                    size = std::min(size, run.length() - file_offset);
                    run.file()->Read(&data, 0, run.offset() + file_offset, size);
                    file_offset += size;
                    candidate_pos = 0;
                }
            }
    };

    void                CreateRuns(SurfelDiskArray& array,
                                   const size_t run_length, 
                                   const uint32_t runs_count);
    void                Merge(SurfelDiskArray& array, const size_t buffer_size);

    size_t              memory_limit_;

    Surfel::CompareFunction
                        compare_;

    SharedFile          runs_file_;

    std::vector<SurfelDiskArray>
                        runs_;
};

} } // namespace lamure

#endif // PRE_EXTERNAL_SORT_H_

