// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_SERIALIZED_SURFEL_H_
#define PRE_SERIALIZED_SURFEL_H_

#include <lamure/types.h>
#include <lamure/pre/surfel.h>
#include <cstring>

namespace lamure {
namespace pre
{

class SerializedSurfel /*final*/
{
public:
                        SerializedSurfel() {
                            data_ = { 0.f, 0.f, 0.f,
                                      0u,  0u,  0u,  0u,
                                      0.f,
                                      0.f, 0.f, 0.f };
                        }

                        SerializedSurfel(const Surfel& surfel) {    
                            SetSurfel(surfel);
                        }

                        SerializedSurfel& operator=(const Surfel& other) {
                            SetSurfel(other);
                            return *this;
                        }

    static const size_t GetSize() { return sizeof(Data); };

    bool                operator==(const SerializedSurfel& rhs) const
                            { return std::memcmp(raw_data_, 
                                                 rhs.raw_data_, 
                                                 sizeof(Data)) == 0; }

    bool                operator!=(const SerializedSurfel& rhs) const
                            { return !(operator==(rhs)); }

    void SetSurfel(const Surfel& surfel) {
        data_ = {
            float(surfel.pos().x),    float(surfel.pos().y),    float(surfel.pos().z),
            surfel.color().x,  surfel.color().y,  surfel.color().z, 0,
            float(surfel.radius()),
            surfel.normal().x, surfel.normal().y, surfel.normal().z };
    }

    Surfel GetSurfel() const {
        return Surfel(vec3r(data_.x, data_.y, data_.z),
                      vec3b(data_.r, data_.g, data_.b),
                      data_.size,
                      vec3f(data_.nx, data_.ny, data_.nz));
    }

    void Serialize(char *data) {
        std::memcpy(data, raw_data_, GetSize());
    }

    SerializedSurfel& Deserialize(char *data) {
        std::memcpy(raw_data_, data, GetSize());
        return *this;
    }

private:
    
    struct Data
    {
        float x, y, z;
        uint8_t r, g, b, fake;
        float size;
        float nx, ny, nz;
    };

    union {
      Data    data_;
      uint8_t raw_data_[sizeof(Data)];
    };

};


} } // namespace lamure


#endif // PRE_SERIALIZED_SURFEL_H_

