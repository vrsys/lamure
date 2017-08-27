#ifndef LAMURE_POINTMETADATAPAIR_H
#define LAMURE_POINTMETADATAPAIR_H

#include "lamure/pro/data/entities/MetaData.h"
#include "lamure/pro/data/entities/Point.h"

namespace prov
{
template <class TPoint, class TMetaData>
class PointMetaDataPair
{
  public:
    PointMetaDataPair(TPoint point, TMetaData meta_data)
    {
        static_assert(std::is_base_of<Point, TPoint>::value, "The used point type is not a derivative of Point");
        static_assert(std::is_base_of<MetaData, TMetaData>::value, "The used meta data type is not a derivative of MetaData");
        this->_point = point;
        this->_metadata = meta_data;
    }

    TPoint get_point() const { return this->_point; }
    TMetaData get_metadata() const { return this->_metadata; }
  private:
    TPoint _point;
    TMetaData _metadata;
};
};
#endif // LAMURE_POINTMETADATAPAIR_H
