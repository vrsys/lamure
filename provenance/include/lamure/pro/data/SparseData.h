#ifndef LAMURE_SPARSE_DATA_H
#define LAMURE_SPARSE_DATA_H

#include "Data.h"
#include "SparsePoint.h"
#include "lamure/pro/common.h"

namespace prov
{
class SparseData : public Data<SparsePoint>
{
  public:
    SparseData() { _cameras = vec<Camera>(); };
    ~SparseData(){};
    const vec<Camera> &get_cameras() const { return _cameras; }
    friend ifstream &operator>>(ifstream &is, SparseData &data)
    {
        data.read_header(is);
        data.read_cameras(is);
        data.read_points(is);
        return is;
    }

    ifstream &read_cameras(ifstream &is)
    {
        uint16_t cameras_length;
        is.read(reinterpret_cast<char *>(&cameras_length), 2);

        cameras_length = swap(cameras_length, true);

        if(DEBUG)
            printf("\nCameras length: %i", cameras_length);

        for(uint16_t i = 0; i < cameras_length; i++)
        {
            Camera camera = Camera();
            is >> camera;
            camera.prepare();
            _cameras.push_back(camera);
        }

        return is;
    }

  protected:
    vec<Camera> _cameras;
};
}

#endif // LAMURE_SPARSE_DATA_H