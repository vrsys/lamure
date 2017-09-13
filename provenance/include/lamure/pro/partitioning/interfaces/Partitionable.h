#ifndef LAMURE_PARTITIONABLE_H
#define LAMURE_PARTITIONABLE_H

#include "lamure/pro/data/interfaces/Cacheable.h"
#include "lamure/pro/partitioning/entities/Partition.h"

namespace prov
{
template <class TPartition>
class Partitionable
{
  public:
    enum Sort
    {
        STD_SORT = 0,
        BOOST_SPREADSORT = 1,
        PDQ_SORT = 2
    };

    Partitionable()
    {
        this->_partitions = vec<TPartition>();
        this->_sort = STD_SORT;
    }

    virtual void partition() = 0;
    vec<TPartition> const &get_partitions() { return _partitions; }

  protected:
    vec<TPartition> _partitions;
    Sort _sort;
    uint8_t _max_depth = 10;
    uint8_t _min_per_node = 1;
};
};

#endif // LAMURE_PARTITIONABLE_H