#ifndef LAMURE_CAMERA_H
#define LAMURE_CAMERA_H

#include "MetaContainer.h"
#include "lamure/pro/common.h"

namespace prov
{
class Camera : public MetaContainer
{
  public:
    class Frustum;

    Camera(long _index, const string &_im_file_name, const quatd &_orientation, const vec3d &_translation, const vec<char> &_metadata);
    ~Camera();

    quatd get_orientation();
    vec3d get_translation();

    void set_frustum_vertices(const arr<vec3d, 8> &_frustum_vertices);
    const arr<vec3d, 8> &get_frustum_vertices();

  private:
    void prepare();
    void read_image();
    void calculate_frustum();

    long _index;
    double _focal_length;
    string _im_file_name;
    quatd _orientation;
    vec3d _translation;
    int _im_height;
    int _im_width;
    double _fp_resolution_x;
    double _fp_resolution_y;
    arr<vec3d, 8> _frustum_vertices;
};
}

#endif // LAMURE_CAMERA_H
