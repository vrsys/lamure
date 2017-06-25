#ifndef LAMURE_SPARSE_DATA_H
#define LAMURE_SPARSE_DATA_H

#include "lamure/pro/common.h"
#include "lamure/pro/data/entities/SparsePoint.h"
#include "lamure/pro/data/interfaces/Cacheable.h"

namespace prov
{
class SparseCache : public Cacheable<SparsePoint>
{
  public:

    SparseCache(ifstream &is_prov, ifstream &is_meta) : Cacheable(is_prov, is_meta)
    {
        _cameras = vec<Camera>();
        _cameras_metadata = vec<MetaData>();
    };

    void cache() override
    {
        Cacheable::cache();
        cache_cameras();
    }

    const vec<prov::Camera> &get_cameras() const { return _cameras; }
    const vec<prov::MetaData> &get_cameras_metadata() const { return _cameras_metadata; }

  protected:
    void cache_cameras()
    {
        uint16_t cameras_length;
        (*is_prov).read(reinterpret_cast<char *>(&cameras_length), 2);
        cameras_length = swap(cameras_length, true);

        // if(DEBUG)
            printf("\nCameras length: %i", cameras_length);

        uint32_t meta_data_length;
        (*is_prov).read(reinterpret_cast<char *>(&meta_data_length), 4);
        meta_data_length = swap(meta_data_length, true);

        // if(DEBUG)
            printf("\nCamera meta data length: %i ", meta_data_length);

        uint16_t max_len_fpath;
        (*is_prov).read(reinterpret_cast<char *>(&max_len_fpath), 2);
        max_len_fpath = swap(max_len_fpath, true);

        // if(DEBUG)
            printf("\nMax file path length: %i ", max_len_fpath);

        for(uint16_t i = 0; i < cameras_length; i++)
        {
            Camera camera = Camera();
            camera.MAX_LENGTH_FILE_PATH = max_len_fpath;
            (*is_prov) >> camera;
            camera.prepare();
            _cameras.push_back(camera);

            MetaData meta_container;
            meta_container.read_metadata((*is_meta), meta_data_length);
            _cameras_metadata.push_back(meta_container);
        }
    }

    vec<Camera> _cameras;
    vec<MetaData> _cameras_metadata;
};
}

#endif // LAMURE_SPARSE_DATA_H