#ifndef LAMURE_DENSE_DATA_H
#define LAMURE_DENSE_DATA_H

#include <lamure/prov/common.h>
#include <lamure/prov/data/entities/DensePoint.h>
#include <lamure/prov/data/entities/DenseMetaData.h>
#include <lamure/prov/data/interfaces/Cacheable.h>
#include <lamure/prov/data/interfaces/Streamable.h>

namespace lamure {
namespace prov
{
class DenseCache : public Cacheable<DensePoint, DenseMetaData>
{
  public:
    DenseCache(ifstream &is_prov, ifstream &is_meta) : Cacheable(is_prov, is_meta){};
    ~DenseCache(){};
};
}
}

#endif // LAMURE_DENSE_DATA_H