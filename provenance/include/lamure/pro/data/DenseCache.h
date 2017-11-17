#ifndef LAMURE_DENSE_DATA_H
#define LAMURE_DENSE_DATA_H

#include "lamure/pro/common.h"
#include "lamure/pro/data/entities/DensePoint.h"
#include "lamure/pro/data/entities/DenseMetaData.h"
#include "lamure/pro/data/interfaces/Cacheable.h"
#include "lamure/pro/data/interfaces/Streamable.h"

namespace prov
{
class DenseCache : public Cacheable<DensePoint, DenseMetaData>
{
  public:
    DenseCache(ifstream &is_prov, ifstream &is_meta) : Cacheable(is_prov, is_meta){};
    ~DenseCache(){};
};
}

#endif // LAMURE_DENSE_DATA_H