#ifndef LAMURE_CACHEABLE_DATA_H
#define LAMURE_CACHEABLE_DATA_H

#include "Readable.h"
#include "lamure/pro/common.h"
#include "lamure/pro/data/entities/Point.h"
#include "lamure/pro/data/interfaces/Cacheable.h"

namespace prov
{
template <class TPoint>
class Cacheable : public Readable
{
  public:
    Cacheable(ifstream &is_prov, ifstream &is_meta)
    {
        this->is_prov = u_ptr<ifstream>(&is_prov);
        this->is_meta = u_ptr<ifstream>(&is_meta);
        static_assert(std::is_base_of<Point, TPoint>::value, "The used point type is not a derivative of Point");
        _points = vec<TPoint>();
        _points_metadata = vec<MetaData>();
    }
    ~Cacheable(){};

    void cache_points()
    {
        uint32_t points_length;
        (*is_prov).read(reinterpret_cast<char *>(&points_length), 4);

        points_length = swap(points_length, true);

        if(DEBUG)
            printf("\nPoints length: %i", points_length);

        uint32_t meta_data_length;
        (*is_prov).read(reinterpret_cast<char *>(&meta_data_length), 4);
        meta_data_length = swap(meta_data_length, true);

        if(DEBUG)
            printf("\nPoints meta data length: %i ", meta_data_length);

        for(int i = 0; i < points_length; i++)
        {
            TPoint point = TPoint();
            (*is_prov) >> point;
            _points.push_back(point);

            MetaData meta_container;
            meta_container.read_metadata((*is_meta), meta_data_length);
            _points_metadata.push_back(meta_container);
        }
    }

    const vec<TPoint> &get_points() const { return _points; }
    const vec<prov::MetaData> &get_points_metadata() const { return _points_metadata; }

    virtual void cache()
    {
        read_header((*is_prov));
        read_header((*is_meta));

        cache_points();
    }

  protected:
    u_ptr<ifstream> is_prov, is_meta;
    vec<TPoint> _points;
    vec<MetaData> _points_metadata;
};
}

#endif // LAMURE_CACHEABLE_DATA_H