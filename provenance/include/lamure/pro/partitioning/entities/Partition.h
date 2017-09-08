#ifndef LAMURE_PARTITION_H
#define LAMURE_PARTITION_H

#include "lamure/pro/data/entities/MetaData.h"
#include "lamure/pro/data/entities/Point.h"

namespace prov
{
template <class TPair, class TMetaData>
class Partition
{
  public:
    Partition()
    {
        this->_pairs = vec<TPair>();
        this->_aggregate_metadata = TMetaData();
    }

    void set_pairs(vec<TPair> pairs) { this->_pairs = pairs; }
    TMetaData get_aggregate_metadata() { return this->_aggregate_metadata; };
  protected:
    vec<TPair> _pairs;
    TMetaData _aggregate_metadata;
};
};
#endif // LAMURE_PARTITION_H
