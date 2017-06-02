#include "lamure/pro/data/Camera.h"

namespace prov
{
Camera::Camera(long _index, const string &_im_file_name, const quatd &_orientation, const vec3d &_translation, const vec<char> &_metadata)
    : MetaContainer(_metadata), _index(_index), _im_file_name(_im_file_name), _orientation(_orientation), _translation(_translation)
{
    prepare();
}

Camera::~Camera() {}
void Camera::prepare()
{
    read_image();
    calculate_frustum();
}

quatd Camera::get_orientation() { return _orientation; }
vec3d Camera::get_translation() { return _translation; }
void Camera::read_image()
{
    FILE *fp = fopen(_im_file_name.c_str(), "rb");
    if(!fp)
    {
        printf("Can't open file: %s\n", _im_file_name.c_str());
    }
    fseek(fp, 0, SEEK_END);
    size_t fsize = (size_t)ftell(fp);
    rewind(fp);
    unsigned char *buf = new unsigned char[fsize];
    if(fread(buf, 1, fsize, fp) != fsize)
    {
        printf("Can't read file: %s\n", _im_file_name.c_str());
        delete[] buf;
    }
    fclose(fp);

    easyexif::EXIFInfo result;
    result.parseFrom(buf, (unsigned int)fsize);

    _focal_length = result.FocalLength * 0.001;
    _fp_resolution_x = result.LensInfo.FocalPlaneResolutionUnit == 2 ? result.LensInfo.FocalPlaneXResolution / 0.0254 : result.LensInfo.FocalPlaneXResolution / 0.01;
    _fp_resolution_y = result.LensInfo.FocalPlaneResolutionUnit == 2 ? result.LensInfo.FocalPlaneYResolution / 0.0254 : result.LensInfo.FocalPlaneYResolution / 0.01;

    //    printf("Focal length: %f, FP Resolution X: %f, Y: %f\n", focal_length_m, resolution_x, resolution_y);
}

void Camera::calculate_frustum()
{
    _frustum_vertices = arr<vec3d, 8>();

    double width_world_half = (1.0 / _fp_resolution_x) * _im_width / 2;
    double height_world_half = (1.0 / _fp_resolution_y) * _im_height / 2;

    _frustum_vertices[0] = vec3d(0.0, 0.0, 0.0);
    _frustum_vertices[1] = vec3d(0.0, 0.0, 0.0);
    _frustum_vertices[2] = vec3d(0.0, 0.0, 0.0);
    _frustum_vertices[3] = vec3d(0.0, 0.0, 0.0);
    _frustum_vertices[4] = vec3d(-width_world_half, height_world_half, -_focal_length);
    _frustum_vertices[5] = vec3d(width_world_half, height_world_half, -_focal_length);
    _frustum_vertices[6] = vec3d(-width_world_half, -height_world_half, -_focal_length);
    _frustum_vertices[7] = vec3d(width_world_half, -height_world_half, -_focal_length);
}

const arr<vec3d, 8> &Camera::get_frustum_vertices() { return _frustum_vertices; }
void Camera::set_frustum_vertices(const arr<vec3d, 8> &_frustum_vertices) { this->_frustum_vertices = _frustum_vertices; }
}