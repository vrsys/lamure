#include <lamure/memory_status.h>
#include <lamure/memory.h>
#include <cassert>

namespace lamure
{

memory_status::
memory_status(const float mem_ratio, const size_t element_size)
    : element_size_(element_size)
{
    memory_budget_ = get_total_memory() * mem_ratio;
}

const size_t memory_status::
max_elements_allowed() const
{
    const size_t occupied = get_total_memory() - get_available_memory();
    if (occupied >= memory_budget_)
        return 0;

    assert(element_size_ > 0);

    return (memory_budget_ - occupied) / element_size_;
}

const size_t memory_status::
max_elements_in_buffer(const size_t buffer_size_mb) const
{
    return (buffer_size_mb * 1024 * 1024) / element_size_;
}

} // namespace lamure

