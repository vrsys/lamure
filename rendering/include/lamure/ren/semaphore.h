// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef REN_SEMAPHORE_H_
#define REN_SEMAPHORE_H_

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <lamure/ren/platform.h>

namespace lamure {
namespace ren {

class RENDERING_DLL semaphore
{
public:
                        semaphore();
    virtual             ~semaphore();

    void                wait();
    void                Signal(const size_t signal_count);
    void                Lock();
    void                Unlock();
    void                Shutdown();

    inline const size_t max_signal_count() const { return max_signal_count_; };
    inline const size_t min_signal_count() const { return min_signal_count_; };
    inline void         set_max_signal_count(size_t max_signal_count) { max_signal_count_ = max_signal_count; };
    inline void         set_min_signal_count(size_t min_signal_count) { min_signal_count_ = min_signal_count; };

    const size_t        NumSignals();

private:
    /* data */
    size_t              signal_count_;

    std::mutex          mutex_;

    std::condition_variable signal_lock_;
    bool                shutdown_;

    size_t              min_signal_count_;
    size_t              max_signal_count_;
};


} } // namespace lamure


#endif // REN_SEMAPHORE_H_

