// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/external_sort.h>

#if WIN32
#include <ppl.h>
#else
#include <parallel/algorithm>
#endif

#include <numeric>

namespace lamure
{
namespace pre
{

const uint32_t MAX_RUNS_COUNT = 512;

const size_t MIN_MERGE_BUFFER_SIZE = 3;

const std::string TEMP_FILE_EXT = ".runs";

external_sort::
external_sort(const size_t memory_limit,
              const surfel::compare_function &compare)
    : memory_limit_(memory_limit),
      compare_(compare),
      runs_file_(std::make_shared<surfel_file>())
{}

void external_sort::
sort(surfel_disk_array &array,
     const size_t memory_limit,
     const surfel::compare_function &compare)
{
    assert(!array.is_empty());
    assert(array.get_file());

    if (!array.length())
        return;

    external_sort es(memory_limit, compare);

    // compute sort parameters
    const size_t run_length = memory_limit / sizeof(surfel) / 3u;
    const uint32_t runs_count = std::ceil(array.length() / double(run_length));
    const size_t merge_buffer_size = memory_limit / sizeof(surfel) / (runs_count + 1u);

    LOGGER_INFO("External sort. Length: " << array.length());
    LOGGER_INFO("Max run length: " << run_length <<
                                   " surfels. runs: " << runs_count <<
                                   ". merge buffer size: " << merge_buffer_size << " surfels.");


    if (runs_count > MAX_RUNS_COUNT) {
        LOGGER_WARN("External sort has been called with an inadequate "
                        "memory limit, which produces more than " <<
                                                                  MAX_RUNS_COUNT << " runs.");
    }

    if (merge_buffer_size < MIN_MERGE_BUFFER_SIZE)
        LOGGER_WARN("External sort has been called with an inadequate "
                        "memory limit, which forces merge algorithm to allocate "
                        "buffers that store less than " <<
                                                        MIN_MERGE_BUFFER_SIZE << " surfels.");

    if (runs_count > 1u) {
        // external sort
        es.runs_file_->open(array.get_file()->file_name() + TEMP_FILE_EXT, true);
        LOGGER_TRACE("create runs");
        es.create_runs(array, run_length, runs_count);
        LOGGER_TRACE("merge");
        es.merge(array, merge_buffer_size);
        es.runs_file_->close(true);
        es.runs_.clear();
    }
    else {
        // internal sort for a single run
        shared_surfel_vector data = array.read_all();

#if WIN32
        Concurrency::parallel_sort(data->begin(), data->end(), es.compare_);
#else
        __gnu_parallel::sort(data->begin(), data->end(), es.compare_);
#endif
        array.write_all(data, 0);
    }
}

void external_sort::
create_runs(surfel_disk_array &array,
            const size_t run_length,
            const uint32_t runs_count)
{
    // construct runs' surfel_disk_arrays
    size_t offset = 0;
    for (uint32_t i = 0; i < runs_count - 1; ++i) {
        runs_.push_back(surfel_disk_array(array, array.offset() + offset, run_length));
        offset += run_length;
    }
    runs_.push_back(surfel_disk_array(array, array.offset() + offset,
                                      array.length() - offset));

    assert(std::accumulate(runs_.begin(), runs_.end(), 0u,
                           [](const size_t &a,
                              const surfel_disk_array &b)
                           { return a + b.length(); }) ==
        array.length());
    // sort the runs
    shared_surfel_vector next_data;

    for (uint32_t i = 0; i < runs_.size(); ++i) {

        shared_surfel_vector data;

        if (next_data) {
            data = next_data;
            next_data.reset();
        }
        else {
            LOGGER_TRACE("read run " << i);
            data = runs_[i].read_all();
        }

#pragma omp parallel sections
        {
            {
                LOGGER_TRACE("sort run " << i);
#if WIN32
                Concurrency::parallel_sort(data->begin(), data->end(), compare_);
#else
                __gnu_parallel::sort(data->begin(), data->end(), compare_);
#endif
            }
#pragma omp section
            {
                if (i + 1 < runs_.size()) {
                    LOGGER_TRACE("read run " << i + 1);
                    next_data = runs_[i + 1].read_all();
                }
            }
        }
        LOGGER_TRACE("Save run " << i);
        runs_file_->append(&(*data));
        runs_[i].reset(runs_file_, runs_[i].offset() - array.offset(),
                       runs_[i].length());
    }
}

void external_sort::
merge(surfel_disk_array &array, const size_t buffer_size)
{
    size_t file_offset = 0;
    surfel_vector output;
    output.reserve(buffer_size);

    std::vector<buffer> buffers;
    for (auto r: runs_)
        buffers.push_back(buffer(r, buffer_size));

    surfel least;
    int least_idx;

    do {
        least_idx = -1;
        for (size_t i = 0; i < buffers.size(); ++i) {
            surfel current_surfel;

            if (buffers[i].front(current_surfel) && (least_idx == -1 ||
                compare_(current_surfel, least))) {
                least = current_surfel;
                least_idx = i;
            }
        }
        if (least_idx != -1) {
            buffers[least_idx].pop_front();
            output.push_back(least);
            if (output.size() >= buffer_size) {
                array.get_file()->write(&output, 0, array.offset() + file_offset,
                                    output.size());
                file_offset += output.size();
                output.clear();
            }
        }
    }
    while (least_idx != -1);

    if (output.size() > 0) {
        array.get_file()->write(&output, 0, array.offset() + file_offset,
                            output.size());
        file_offset += output.size();
        output.clear();
    }
    assert(file_offset == array.length());
}

}
} // namespace lamure

