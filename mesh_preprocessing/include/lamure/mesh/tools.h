
#ifndef LAMURE_MESH_TOOLS_H_
#define LAMURE_MESH_TOOLS_H_

#include <vector>
#include <functional>
#include <thread>
#include <future>
#include <iostream>

namespace lamure
{
namespace mesh
{
static void show_progress(size_t _current_item, size_t _total_items)
{
    if(_total_items == 0)
    {
        return;
    }
    float percentage = (float)(((float)(_current_item) / (float)_total_items) * 100.f);
    if((_current_item % (_total_items / 1000)) == 0)
    {
        uint32_t progress = (uint32_t)((size_t)(((double)(_current_item) / (double)_total_items) * 25.0));
        std::cout << "\rPROGRESS: [";
        for(uint32_t p = 0; p < 25; p++)
        {
            if(p <= progress)
                std::cout << "#";
            else
                std::cout << ".";
        }
        std::cout << "] " << percentage << "% (" << _current_item << " done) " << std::flush;
    }
}

enum schedule_t
{
    STATIC = 0,
    DYNAMIC = 1
};

template <typename F>
static void parallel_for(uint32_t num_threads, uint64_t num_items, F& task)
{
    parallel_for(schedule_t::STATIC, num_threads, num_items, task, true);
}

template <typename F>
static void parallel_for(schedule_t schedule, uint32_t num_threads, uint64_t num_items, F& task, bool _verbose = true)
{
    if(num_items == 0)
    {
        return;
    }
    if(num_items == 1)
    {
        task(0, 0);
        return;
    }

    if(_verbose)
    {
        std::cout << "LOG: start parallel for (" << num_threads << " threads, " << num_items << " items)" << std::endl;
    }

    uint32_t n_threads = (uint32_t)std::min((size_t)num_threads, num_items);
    std::vector<std::thread> threads;

    switch(schedule)
    {
    case schedule_t::DYNAMIC:
    {
        uint64_t current_item = 0;
        std::mutex mutex;
        for(uint32_t id = 0; id < n_threads; ++id)
        {
            threads.push_back(std::thread([&, id]() -> void {
                mutex.lock();
                while(current_item < num_items)
                {
                    uint64_t item = current_item++;
                    mutex.unlock();
                    task(item, id);
                    mutex.lock();
                }
                mutex.unlock();
            }));
        }
        break;
    }
    case schedule_t::STATIC:
    {
        size_t num_items_per_thread = num_items / n_threads + (num_items % n_threads != 0);
        for(uint32_t id = 0; id < n_threads; ++id)
        {
            threads.push_back(std::thread([&, id]() -> void {
                size_t start = id * num_items_per_thread;

                size_t stride = std::min(num_items_per_thread, num_items - start);
                for(uint64_t item = start; item < start + stride; ++item)
                {
                    if(item < num_items)
                    {
                        task(item, id);
                    }
                }
            }));
        }
        break;
    }
    default:
        break;
    }

    for(auto& thread : threads)
    {
        if(thread.joinable())
        {
            thread.join();
        }
    }
}

} // namespace mesh
} // namespace lamure

#endif