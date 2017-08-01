#ifndef LAMURE_LODMETADATA_H
#define LAMURE_LODMETADATA_H

#include "MetaData.h"
#include "lamure/pro/common.h"

namespace prov
{
class DenseMetaData: public MetaData
{
public:
    DenseMetaData()
    {
        images_seen = vec<int>();
        images_not_seen = vec<int>();
    }

    virtual void read_metadata(ifstream &is, uint32_t meta_data_length) override
    {
        MetaData::read_metadata(is, meta_data_length);

        uint32_t data_pointer = 0;

        const char *buffer = swap(reinterpret_cast<char *>(&_metadata[data_pointer], 8), true);
        photometric_consistency = atof(buffer);
        data_pointer += 8;

        printf("\nNCC: %f", photometric_consistency);

        //        uint32_t num_seen = atoi(reinterpret_cast<char *>(&_metadata[data_pointer], 4));
        //        num_seen = swap(num_seen, true);
        //        data_pointer += 4;
        //
        //        printf("\nNum seen: %i", num_seen);
        //
        //        for(uint32_t i = 0; i < num_seen; i++)
        //        {
        //            uint32_t image_seen = atoi(reinterpret_cast<char *>(&_metadata[data_pointer], 4));
        //            image_seen = swap(image_seen, true);
        //            data_pointer += 4;
        //
        //            printf("\nImage seen: %i", image_seen);
        //
        //            images_seen.push_back(image_seen);
        //        }
        //
        //        uint32_t num_not_seen = atoi(reinterpret_cast<char *>(&_metadata[data_pointer], 4));
        //        num_not_seen =  swap(num_not_seen, true);
        //        data_pointer += 4;
        //
        //        printf("\nNum not seen: %i", num_not_seen);
        //
        //        for(uint32_t i = 0; i < num_seen; i++)
        //        {
        //            uint32_t image_not_seen = atoi(reinterpret_cast<char *>(&_metadata[data_pointer], 4));
        //            image_not_seen = swap(image_not_seen, true);
        //            data_pointer += 4;
        //
        //            printf("\nImage not seen: %i", image_not_seen);
        //
        //            images_not_seen.push_back(image_not_seen);
        //        }
    }

private:
    float photometric_consistency;
    vec<int> images_seen;
    vec<int> images_not_seen;
};
};

#endif // LAMURE_LODMETADATA_H
