// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_UTIL_MEMORY_STATUS_H_
#define LAMURE_UTIL_MEMORY_STATUS_H_

#include <lamure/platform_core.h>
#include <lamure/types.h>
#include <cstddef>


namespace lamure {
namespace util {

class CORE_DLL memory_status_t
{
public:
    explicit            memory_status_t(const float32_t mem_ratio, const size_t element_size);
    virtual             ~memory_status_t() {};

    const size_t        memory_budget() const { return memory_budget_; };
    void                set_memory_budget(const size_t value)
                            { memory_budget_ = value; };

    const size_t        element_size() const { return element_size_; };
    void                set_element_size(const size_t value)
                            { element_size_ = value; };

    const size_t        max_elements_allowed() const;

    const size_t        max_elements_in_buffer(const size_t buffer_size_mb) const;

private:

    size_t              memory_budget_;
    size_t              element_size_;

};

} // namespace util
} // namespace lamure

#endif // LAMURE_UTIL_MEMORY_MANAGER_H_
