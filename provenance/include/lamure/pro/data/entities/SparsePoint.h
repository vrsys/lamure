#ifndef LAMURE_SPARSE_POINT_H
#define LAMURE_SPARSE_POINT_H

#include "Camera.h"
#include "Point.h"
#include "lamure/pro/common.h"
#include "MetaData.h"

namespace prov
{
class SparsePoint : public Point
{
  public:
    class Measurement
    {
      public:
        Measurement() { _occurence = vec2d(); }
        Measurement(uint16_t _camera_index, const vec2d &_occurence) : _camera_index(_camera_index), _occurence(_occurence) {}
        ~Measurement() {}
        const uint16_t &get_camera() const { return _camera_index; }
        const vec2d &get_occurence() const { return _occurence; };
        friend ifstream &operator>>(ifstream &is, Measurement &measurement)
        {
            is.read(reinterpret_cast<char *>(&measurement._camera_index), 2);

            measurement._camera_index = swap(measurement._camera_index, true);

            if(DEBUG)
                printf("\nCamera index: %i", measurement._camera_index);

            is.read(reinterpret_cast<char *>(&measurement._occurence.x), 8);
            measurement._occurence.x = swap(measurement._occurence.x, true);
            is.read(reinterpret_cast<char *>(&measurement._occurence.y), 8);
            measurement._occurence.y = swap(measurement._occurence.y, true);

            if(DEBUG)
                printf("\nOccurence: %f %f", measurement._occurence.x, measurement._occurence.y);

            return is;
        }

      private:
        uint16_t _camera_index;
        vec2d _occurence;
    };

    SparsePoint() { _measurements = vec<Measurement>(); }
    SparsePoint(uint32_t _index, const vec3d &_center, const vec3d &_color, const vec<uint8_t> &_metadata, const vec<Measurement> &_measurements)
        : Point(_center, _color, _metadata), _measurements(_measurements)
    {
    }
    ~SparsePoint() {}
    const vec<Measurement> &get_measurements() const { return _measurements; }
    friend ifstream &operator>>(ifstream &is, SparsePoint &sparse_point)
    {
        is.read(reinterpret_cast<char *>(&sparse_point._index), 4);
        sparse_point._index = swap(sparse_point._index, true);

        if(DEBUG)
            printf("\nPoint index: %i", sparse_point._index);

        sparse_point.read_essentials(is);

        uint16_t measurements_length;
        is.read(reinterpret_cast<char *>(&measurements_length), 2);

        measurements_length = swap(measurements_length, true);

        if(DEBUG)
            printf("\nMeasurements length: %i", measurements_length);

        for(uint16_t i = 0; i < measurements_length; i++)
        {
            Measurement measurement = Measurement();
            is >> measurement;
            sparse_point._measurements.push_back(measurement);
        }

        return is;
    }

    uint32_t get_index() const { return _index; }
protected:
    uint32_t _index;
    vec<Measurement> _measurements;
};
}

#endif // LAMURE_SPARSE_POINT_H