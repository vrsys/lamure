// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef COMMON_MEMORY_MANAGER_H_
#define COMMON_MEMORY_MANAGER_H_

#include <lamure/platform.h>
#include <cstddef>

namespace lamure
{

class COMMON_DLL MemoryStatus
{
public:
    explicit            MemoryStatus(const float mem_ratio, const size_t element_size);
    virtual             ~MemoryStatus() {};

    const size_t        memory_budget() const { return memory_budget_; };
    void                set_memory_budget(const size_t value)
                            { memory_budget_ = value; };

    const size_t        element_size() const { return element_size_; };
    void                set_element_size(const size_t value)
                            { element_size_ = value; };

    const size_t        MaxElementsAllowed() const;

    const size_t        MaxElementsInBuffer(const size_t buffer_size_mb) const;

private:

    size_t              memory_budget_;
    size_t              element_size_;

};

} // namespace lamure

#endif // COMMON_MEMORY_MANAGER_H_
