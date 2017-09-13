#ifndef LAMURE_LODMETADATA_H
#define LAMURE_LODMETADATA_H

#include "MetaData.h"
#include "lamure/pro/common.h"

namespace prov
{
class DenseMetaData : public MetaData
{
  public:
    DenseMetaData()
    {
        images_seen = vec<uint32_t>();
        images_not_seen = vec<uint32_t>();
    }
    ~DenseMetaData(){}

    virtual void read_metadata(ifstream &is, uint32_t meta_data_length) override
    {
        MetaData::read_metadata(is, meta_data_length);

        uint32_t data_pointer = 0;

        float buffer = 0;
        memcpy(&buffer, &_metadata[data_pointer], 4);
        photometric_consistency = (float)swap(buffer, true);
        data_pointer += 4;

        // printf("\nNCC: %f", photometric_consistency);

        uint32_t num_seen = 0;
        memcpy(&num_seen, &_metadata[data_pointer], 4);
        num_seen = swap(num_seen, true);
        data_pointer += 4;

        // printf("\nNum seen: %i", num_seen);

        for(uint32_t i = 0; i < num_seen; i++)
        {
            uint32_t image_seen = 0;
            memcpy(&image_seen, &_metadata[data_pointer], 4);
            image_seen = swap(image_seen, true);
            data_pointer += 4;

            //            printf("\nImage seen: %i", image_seen);

            images_seen.push_back(image_seen);
        }

        uint32_t num_not_seen = 0;
        memcpy(&num_not_seen, &_metadata[data_pointer], 4);
        num_not_seen = swap(num_not_seen, true);
        data_pointer += 4;

        // printf("\nNum not seen: %i", num_not_seen);

        for(uint32_t i = 0; i < num_not_seen; i++)
        {
            uint32_t image_not_seen = 0;
            memcpy(&image_not_seen, &_metadata[data_pointer], 4);
            image_not_seen = swap(image_not_seen, true);
            data_pointer += 4;

            //            printf("\nImage not seen: %i", image_not_seen);

            images_not_seen.push_back(image_not_seen);
        }
    }

    float get_photometric_consistency() const { return photometric_consistency; }
    vec<uint32_t> get_images_seen() const { return images_seen; }
    vec<uint32_t> get_images_not_seen() const { return images_not_seen; }
    void set_photometric_consistency(float photometric_consistency) { this->photometric_consistency = photometric_consistency; }
    void set_images_seen(vec<uint32_t> images_seen) { this->images_seen = images_seen; }
    void set_images_not_seen(vec<uint32_t> images_not_seen) { this->images_not_seen = images_not_seen; }
  private:
    float photometric_consistency;
    vec<uint32_t> images_seen;
    vec<uint32_t> images_not_seen;
};
};

#endif // LAMURE_LODMETADATA_H
