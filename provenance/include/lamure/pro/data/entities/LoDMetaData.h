#ifndef LAMURE_LODMETADATA_H
#define LAMURE_LODMETADATA_H

#include "MetaData.h"
#include "lamure/pro/common.h"

namespace prov
{
class LoDMetaData : public MetaData
{
  public:
    LoDMetaData() {}

    double get_mean_absolute_deviation() const { return _mean_absolute_deviation; }
    double get_standard_deviation() const { return _standard_deviation; }
    double get_coefficient_of_variation() const { return _coefficient_of_variation; }

    virtual void read_metadata(ifstream &is, uint64_t meta_data_length) override
    {
        MetaData::read_metadata(is, meta_data_length);
        _mean_absolute_deviation = atof(reinterpret_cast<char *>(&_metadata[0], 8));
        swap(_mean_absolute_deviation, true);
        _standard_deviation = atof(reinterpret_cast<char *>(&_metadata[8], 8));
        swap(_standard_deviation, true);
        _coefficient_of_variation = atof(reinterpret_cast<char *>(&_metadata[16], 8));
        swap(_coefficient_of_variation, true);
    }

    double _mean_absolute_deviation;
    double _standard_deviation;
    double _coefficient_of_variation;

    static const uint32_t ENTITY_LENGTH = 0x18;
};
};

#endif // LAMURE_LODMETADATA_H
