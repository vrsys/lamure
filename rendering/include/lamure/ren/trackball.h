
#ifndef LAMURE_REN_TRACKBALL_H_INCLUDED
#define LAMURE_REN_TRACKBALL_H_INCLUDED

#include <scm/core/math.h>
#include <scm/gl_core/math/math.h>

namespace lamure {
namespace ren {

class Trackball
{
public:
    Trackball();
    ~Trackball();

    void Rotate(double fx, double fy, double tx, double ty);
    void Translate(double x, double y);
    void Dolly(double y);

    const scm::math::mat4d& transform() const { return transform_; };
    void set_transform(const scm::math::mat4d& transform) { transform_ = transform; };
    double dolly() const { return dolly_; };
    void set_dolly(const double dolly) { dolly_ = dolly; };

private:
    double ProjectTosphere(double x, double y) const;

    scm::math::mat4d transform_;
    double radius_;
    double dolly_;

};

}
}

#endif
