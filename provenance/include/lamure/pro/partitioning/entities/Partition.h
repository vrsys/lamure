#ifndef LAMURE_PARTITION_H
#define LAMURE_PARTITION_H

#include "lamure/pro/data/entities/MetaData.h"
#include "lamure/pro/data/entities/Point.h"
#include "lamure/pro/partitioning/entities/PointMetaDataPair.h"

namespace prov
{
template <class TPointMetaDataPair, class TMetaData>
class Partition
{
  public:
    Partition()
    {
        //        static_assert(std::is_base_of<PointMetaDataPair, TPointMetaDataPair>::value, "The used PointMetaDataPair type is not a derivative of PointMetaDataPair");
        this->_pairs = vec<TPointMetaDataPair>();
    }

    void set_pairs(vec<TPointMetaDataPair> pairs) { this->_pairs = pairs; }
    TMetaData get_aggregate_metadata() { return this->_aggregate_metadata; };
  protected:
    vec<TPointMetaDataPair> _pairs;
    TMetaData _aggregate_metadata;
};
};
#endif // LAMURE_PARTITION_H
