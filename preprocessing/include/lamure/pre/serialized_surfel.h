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
            float(surfel.pos().x_),    float(surfel.pos().y_),    float(surfel.pos().z_),
            surfel.color().x_,  surfel.color().y_,  surfel.color().z_, 0,
            float(surfel.radius()),
            surfel.normal().x_, surfel.normal().y_, surfel.normal().z_ };
    }

    surfel get_surfel() const {
        return surfel(vec3r_t(data_.x_, data_.y_, data_.z_),
                      vec3b_t(data_.r_, data_.g_, data_.b_),
                      data_.size_,
                      vec3f_t(data_.nx_, data_.ny_, data_.nz_));
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
        float x_, y_, z_;
        uint8_t r_, g_, b_, fake_;
        float size_;
        float nx_, ny_, nz_;
    };

    union {
      data    data_;
      uint8_t raw_data_[sizeof(data)];
    };

};


} } // namespace lamure


#endif // PRE_SERIALIZED_SURFEL_H_

