// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef PRE_SURFEL_H_
#define PRE_SURFEL_H_

#include <lamure/pre/platform.h>
#include <lamure/types.h>

#include <functional>
#include <memory>
#include <vector>

namespace lamure
{
namespace pre
{
class PREPROCESSING_DLL surfel /*final*/
{
  public:
    using compare_function = std::function<bool(const surfel &left, const surfel &right)>;

    surfel(const vec3r &pos = vec3r(0.0), const vec3b &color = vec3b(0), const real radius = 0.0, const vec3f &normal = vec3f(0.f), const float ncc = 0)
        : pos_(pos), color_(color), radius_(radius), normal_(normal), ncc_(ncc)
    {
    }

    const vec3r pos() const { return pos_; }
    vec3r &pos() { return pos_; }

    const vec3b color() const { return color_; }
    vec3b &color() { return color_; }

    const real radius() const { return radius_; }
    real &radius() { return radius_; }

    const vec3f normal() const { return normal_; }
    vec3f &normal() { return normal_; }

    const float ncc() const { return ncc_; }
    float &ncc() { return ncc_; }

    const vec3r random_point_on_surfel() const;

    bool operator==(const surfel &rhs) const { return pos_ == rhs.pos_ && color_ == rhs.color_ && radius_ == rhs.radius_ && normal_ == rhs.normal_; }

    bool operator!=(const surfel &rhs) const { return !(operator==(rhs)); }

    static bool intersect(const surfel &left_surfel, const surfel &right_surfel);

    static bool compare_x(const surfel &left_surfel, const surfel &right_surfel);
    static bool compare_y(const surfel &left_surfel, const surfel &right_surfel);
    static bool compare_z(const surfel &left_surfel, const surfel &right_surfel);

    static compare_function compare(const uint8_t axis);

  private:
    /*static CGAL::Simple_cartesian<double>::Plane_3
                        create_surfel_plane(const surfel& target_surfel, bool is_left);*/

    vec3r pos_;
    vec3b color_;
    real radius_;
    vec3f normal_;
    float ncc_;
};

using surfel_vector = std::vector<surfel>;
using shared_surfel = std::shared_ptr<surfel>;
using shared_surfel_vector = std::shared_ptr<surfel_vector>;
}
} // namespace lamure

#endif // PRE_SURFEL_H_
