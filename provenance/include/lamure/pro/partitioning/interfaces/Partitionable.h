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
    Partitionable()
    {
        //        static_assert(std::is_base_of<Partition, TPartition>::value, "The used partition type is not a derivative of Partition");
        this->_partitions = vec<TPartition>();
    }

    // Stub
    void partition(){};

  protected:
    vec<TPartition> _partitions;
};
};

#endif // LAMURE_PARTITIONABLE_H