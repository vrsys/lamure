// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_EXTERNAL_SORT_H_
#define PRE_EXTERNAL_SORT_H_

#include <lamure/pre/disk_array.h>
#include <vector>
#include <lamure/pre/logger.h>

namespace lamure
{
namespace pre
{

class PREPROCESSING_DLL external_sort
{
public:

    static void sort(surfel_disk_array &array,
                     const size_t memory_limit,
                     const surfel::compare_function &compare);

private:
    explicit external_sort(const size_t memory_limit,
                           const surfel::compare_function &compare);
    external_sort(const external_sort &) = delete;
    external_sort &operator=(const external_sort &) = delete;

    class buffer
    {
    public:
        surfel_disk_array run;
        surfel_vector data;
        size_t size;
        size_t candidate_pos;
        size_t file_offset;

        buffer(const surfel_disk_array &array, const size_t buffer_size)
            : run(array)
        {
            size = buffer_size;
            candidate_pos = size;
            file_offset = 0;
            data = surfel_vector(size);
        }

        bool front(surfel &s)
        {
            invalidate();
            if (size > 0) {
                s = data[candidate_pos];
                return true;
            }
            else
                return false;
        }

        void pop_front()
        {
            assert(size > 0);
            candidate_pos++;
        }

    private:
        void invalidate()
        {
            if (size > 0 && candidate_pos >= size) {
                if (file_offset >= run.length()) {
                    size = 0;
                    return;
                }

                size = std::min(size, run.length() - file_offset);
                run.get_file()->read(&data, 0, run.offset() + file_offset, size);
                file_offset += size;
                candidate_pos = 0;
            }
        }
    };

    void create_runs(surfel_disk_array &array,
                     const size_t run_length,
                     const uint32_t runs_count);
    void merge(surfel_disk_array &array, const size_t buffer_size);

    size_t memory_limit_;

    surfel::compare_function
        compare_;

    shared_surfel_file runs_file_;

    std::vector<surfel_disk_array>
        runs_;
};

}
} // namespace lamure

#endif // PRE_EXTERNAL_SORT_H_

