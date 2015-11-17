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

namespace lamure {
namespace pre
{

const uint32_t    MAX_RUNS_COUNT = 512;
const size_t      MIN_MERGE_BUFFER_SIZE = 3;
const std::string TEMP_FILE_EXT = ".runs";

ExternalSort::
ExternalSort(const size_t memory_limit,
             const Surfel::CompareFunction& compare)
    : memory_limit_(memory_limit),
      compare_(compare),
      runs_file_(std::make_shared<File>())
{}

void ExternalSort::
Sort(SurfelDiskArray& array,
     const size_t memory_limit,
     const Surfel::CompareFunction& compare)
{
    assert(!array.is_empty());
    assert(array.file());

    if (!array.length())
        return;

    ExternalSort es(memory_limit, compare);
    
    // compute sort parameters
    const size_t run_length = memory_limit / sizeof(Surfel) / 3u;
    const uint32_t runs_count = std::ceil(array.length() / double(run_length));
    const size_t merge_buffer_size = memory_limit / sizeof(Surfel) / (runs_count + 1u);

    LOGGER_INFO("External sort. Length: " << array.length());
    LOGGER_INFO("Max run length: " << run_length << 
            " surfels. Runs: " << runs_count << 
            ". Merge buffer size: " << merge_buffer_size << " surfels.");


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
        es.runs_file_->Open(array.file()->file_name() + TEMP_FILE_EXT, true);
        LOGGER_TRACE("Create runs");
        es.CreateRuns(array, run_length, runs_count);
        LOGGER_TRACE("Merge");
        es.Merge(array, merge_buffer_size);
        es.runs_file_->Close(true);
        es.runs_.clear();
    }
    else {
        // internal sort for a single run
        SharedSurfelVector data = array.ReadAll();

#if WIN32
        Concurrency::parallel_sort(data->begin(), data->end(), es.compare_);
#else
        __gnu_parallel::sort(data->begin(), data->end(), es.compare_);
#endif
        array.WriteAll(data, 0);
    }
}

void ExternalSort::
CreateRuns(SurfelDiskArray& array,
           const size_t run_length, 
           const uint32_t runs_count)
{
    // construct runs' SurfelDiskArrays
    size_t offset = 0;
    for (uint32_t i = 0; i < runs_count - 1; ++i) {
        runs_.push_back(SurfelDiskArray(array, array.offset() + offset, run_length));
        offset += run_length;
    }
    runs_.push_back(SurfelDiskArray(array, array.offset() + offset, 
                                           array.length() - offset));

    assert(std::accumulate(runs_.begin(), runs_.end(), 0u,
                           [](const size_t& a,
                              const SurfelDiskArray& b) { return a + b.length(); }) ==
                                                                 array.length());
    // sort the runs
    SharedSurfelVector next_data;

    for (uint32_t i = 0; i < runs_.size(); ++i) {
        
        SharedSurfelVector data;
        
        if (next_data) {
            data = next_data;
            next_data.reset();
        }
        else {
            LOGGER_TRACE("Read run " << i);
            data = runs_[i].ReadAll();
        }

        #pragma omp parallel sections
        {
            {
                LOGGER_TRACE("Sort run " << i);
#if WIN32
                Concurrency::parallel_sort(data->begin(), data->end(), compare_);           
#else
                __gnu_parallel::sort(data->begin(), data->end(), compare_);
#endif
            }
            #pragma omp section
            {
                if (i + 1 < runs_.size()) {
                    LOGGER_TRACE("Read run " << i+1);
                    next_data = runs_[i + 1].ReadAll();
                }
            }
        }
        LOGGER_TRACE("Save run " << i);
        runs_file_->Append(&(*data));
        runs_[i].Reset(runs_file_, runs_[i].offset() - array.offset(), 
                                   runs_[i].length());
    }
}

void ExternalSort::
Merge(SurfelDiskArray& array, const size_t buffer_size)
{
    size_t file_offset = 0;
    SurfelVector output;
    output.reserve(buffer_size);

    std::vector<Buffer> buffers;
    for (auto r: runs_)
        buffers.push_back(Buffer(r, buffer_size));

    Surfel least;
    int least_idx;

    do {
        least_idx = -1;
        for (size_t i = 0; i < buffers.size(); ++i) {
            Surfel current_surfel;

            if (buffers[i].Front(current_surfel) && (least_idx == -1 || 
                        compare_(current_surfel, least))) {
                least = current_surfel;
                least_idx = i;
            }
        }
        if (least_idx != -1) {
            buffers[least_idx].PopFront();
            output.push_back(least);
            if (output.size() >= buffer_size) {
                array.file()->Write(&output, 0, array.offset() + file_offset, 
                                                output.size());
                file_offset += output.size();
                output.clear();
            }
        }
    } while (least_idx != -1);

    if (output.size() > 0) {
        array.file()->Write(&output, 0, array.offset() + file_offset, 
                                        output.size());
        file_offset += output.size();
        output.clear();
    }
    assert(file_offset == array.length());
}

} } // namespace lamure

