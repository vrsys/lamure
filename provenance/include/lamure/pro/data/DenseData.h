#ifndef LAMURE_DENSE_DATA_H
#define LAMURE_DENSE_DATA_H

#include "Data.h"
#include "DensePoint.h"
#include "lamure/pro/common.h"

namespace prov
{
class DenseData : public Data<DensePoint>
{
  public:
    DenseData(){};
    ~DenseData(){};
};
}

#endif // LAMURE_DENSE_DATA_H