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

class serialized_surfel /*final*/
{
public:
                        serialized_surfel() {
                            data_ = { 0.f, 0.f, 0.f,
                                      0u,  0u,  0u,  0u,
                                      0.f,
                                      0.f, 0.f, 0.f };
                        }

                        serialized_surfel(const surfel& surfel) {    
                            set_surfel(surfel);
                        }

                        serialized_surfel& operator=(const surfel& other) {
                            set_surfel(other);
                            return *this;
                        }

    static const size_t get_size() { return sizeof(data); };

    bool                operator==(const serialized_surfel& rhs) const
                            { return std::memcmp(raw_data_, 
                                                 rhs.raw_data_, 
                                                 sizeof(data)) == 0; }

    bool                operator!=(const serialized_surfel& rhs) const
                            { return !(operator==(rhs)); }

    void set_surfel(const surfel& surfel) {
        data_ = {
            float(surfel.pos().x),    float(surfel.pos().y),    float(surfel.pos().z),
            surfel.color().x,  surfel.color().y,  surfel.color().z, 0,
            float(surfel.radius()),
            surfel.normal().x, surfel.normal().y, surfel.normal().z };
    }

    surfel get_surfel() const {
        return surfel(vec3r(data_.x, data_.y, data_.z),
                      vec3b(data_.r, data_.g, data_.b),
                      data_.size,
                      vec3f(data_.nx, data_.ny, data_.nz));
    }

    void serialize(char *data) {
        std::memcpy(data, raw_data_, get_size());
    }

    serialized_surfel& Deserialize(char *data) {
        std::memcpy(raw_data_, data, get_size());
        return *this;
    }

private:
    
    struct data
    {
        float x, y, z;
        uint8_t r, g, b, fake;
        float size;
        float nx, ny, nz;
    };

    union {
      data    data_;
      uint8_t raw_data_[sizeof(data)];
    };

};


} } // namespace lamure


#endif // PRE_SERIALIZED_SURFEL_H_

