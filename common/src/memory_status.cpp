#include <lamure/memory_status.h>
#include <lamure/memory.h>
#include <cassert>

namespace lamure
{

MemoryStatus::
MemoryStatus(const float mem_ratio, const size_t element_size)
    : element_size_(element_size)
{
    memory_budget_ = GetTotalMemory() * mem_ratio;
}

const size_t MemoryStatus::
MaxElementsAllowed() const
{
    const size_t occupied = GetTotalMemory() - GetAvailableMemory();
    if (occupied >= memory_budget_)
        return 0;

    assert(element_size_ > 0);

    return (memory_budget_ - occupied) / element_size_;
}

const size_t MemoryStatus::
MaxElementsInBuffer(const size_t buffer_size_mb) const
{
    return (buffer_size_mb * 1024 * 1024) / element_size_;
}

} // namespace lamure

