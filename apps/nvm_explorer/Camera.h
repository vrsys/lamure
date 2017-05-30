#ifndef LAMURE_CAMERA_H
#define LAMURE_CAMERA_H

#include <scm/core/math/quat.h>
#include <scm/core/math/vec3.h>
#include <scm/core/math.h>
#include <FreeImagePlus.h>
#include "Image.h"
#include "Frustum.h"

using namespace scm::math;

class Camera
{
    private:

        Image _still_image;
        double _focal_length;
        quat<double> _orientation;
        vec3d _center;
        double _radial_distortion;
        float _scale = 20.0f;


        scm::math::mat4f _transformation = scm::math::mat4f::identity();

        Frustum _frustum;
  
        void update_transformation();

        std::vector<scm::math::vec3f> calc_frustum_points();
    public:
        const Image &get_still_image () const;

        void set_still_image (const Image &_still_image);

        double get_focal_length () const;

        void set_focal_length (double _focal_length);

        const quat<double> &get_orientation () const;

        void set_orientation (const quat<double> &_orientation);

        const vec3d &get_center () const;

        void set_center (const vec<double, 3> &_center);

        double get_radial_distortion () const;

        void set_radial_distortion (double _radial_distortion);

        scm::math::mat4f &get_transformation();


        void update_scale_frustum(scm::shared_ptr<scm::gl::render_device> device, float offset);

        Frustum &get_frustum();

        void bind_texture(scm::shared_ptr<scm::gl::render_context> context);

        Camera ();

        Camera (const Image &_still_image, double _focal_length, const quat<double> &_orientation,
              const vec<double, 3> &_center, double _radial_distortion);

        void init(scm::shared_ptr<scm::gl::render_device> device);
};

#endif //LAMURE_CAMERA_H
