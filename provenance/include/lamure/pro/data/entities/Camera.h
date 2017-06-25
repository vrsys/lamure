#ifndef LAMURE_CAMERA_H
#define LAMURE_CAMERA_H

#include "MetaData.h"
#include "lamure/pro/common.h"
#include <scm/core.h>
#include <scm/core/io/tools.h>
#include <scm/core/pointer_types.h>
#include <scm/core/time/accum_timer.h>
#include <scm/core/time/high_res_timer.h>
#include <scm/log.h>

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/primitives/box.h>
#include <scm/gl_util/primitives/quad.h>
#include <scm/gl_util/viewer/camera.h>

#include <scm/core/math.h>

#include <scm/gl_core/gl_core_fwd.h>
#include <scm/gl_util/primitives/geometry.h>
#include <scm/gl_util/primitives/primitives_fwd.h>

#include <scm/gl_util/font/font_face.h>
#include <scm/gl_util/font/text.h>
#include <scm/gl_util/font/text_renderer.h>

#include <scm/core/platform/platform.h>
#include <scm/core/utilities/platform_warning_disable.h>
#include <scm/gl_util/primitives/geometry.h>

#include <lamure/ren/controller.h>
#include <lamure/ren/cut_database.h>
#include <lamure/ren/model_database.h>
#include <lamure/ren/policy.h>

#include <boost/assign/list_of.hpp>
#include <memory>

#include <fstream>
#include <lamure/types.h>
#include <lamure/utils.h>
#include <map>
#include <vector>

namespace prov
{
class Camera
{
  public:
    uint16_t MAX_LENGTH_FILE_PATH;

    class Frustum;

    Camera() {}
    Camera(uint16_t _index, const string &_im_file_name, const quatd &_orientation, const vec3d &_translation, const vec<uint8_t> &_metadata)
        : _index(_index), _im_file_name(_im_file_name), _orientation(_orientation), _translation(_translation)
    {
        prepare();
    }
    ~Camera() {}
    void prepare()
    {
        try
        {
            read_image();
        }
        catch(const std::runtime_error e)
        {
            printf("\nFailed to read image: %s", e.what());
        }

        update_transformation();
        calculate_frustum();
    }

    quatd get_orientation() { return _orientation; }
    vec3d get_translation() { return _translation; }
    scm::math::mat4f &get_transformation() { return _transformation; }
    scm::gl::vertex_array_ptr get_vertex_array_object_frustum() { return _vertex_array_object_frustum; }
    // void Camera::bind_texture(scm::shared_ptr<scm::gl::render_context> context) { context->bind_texture(_still_image.get_texture(), _still_image.get_state(), 0); }
    const arr<vec3d, 8> &get_frustum_vertices() { return _frustum_vertices; }
    void set_frustum_vertices(const arr<vec3d, 8> &_frustum_vertices) { this->_frustum_vertices = _frustum_vertices; }
    friend ifstream &operator>>(ifstream &is, Camera &camera)
    {
        is.read(reinterpret_cast<char *>(&camera._index), 2);
        camera._index = swap(camera._index, true);

        // if(DEBUG)
            printf("\nIndex: %i", camera._index);

        is.read(reinterpret_cast<char *>(&camera._focal_length), 8);
        camera._focal_length = swap(camera._focal_length, true);

        // if(DEBUG)
            printf("\nFocal length: %f", camera._focal_length);

        float w, x, y, z;
        is.read(reinterpret_cast<char *>(&w), 8);
        is.read(reinterpret_cast<char *>(&x), 8);
        is.read(reinterpret_cast<char *>(&y), 8);
        is.read(reinterpret_cast<char *>(&z), 8);
        w = swap(w, true);
        x = swap(x, true);
        y = swap(y, true);
        z = swap(z, true);

        // if(DEBUG)
            printf("\nWXYZ: %f %f %f %f", w, x, y, z);

        camera._orientation = quatd(w, x, y, z);
        is.read(reinterpret_cast<char *>(&x), 8);
        is.read(reinterpret_cast<char *>(&y), 8);
        is.read(reinterpret_cast<char *>(&z), 8);
        x = swap(x, true);
        y = swap(y, true);
        z = swap(z, true);

        // if(DEBUG)
            printf("\nXYZ: %f %f %f", x, y, z);

        camera._translation = vec3d(x, y, z);

        char byte_buffer[camera.MAX_LENGTH_FILE_PATH];
        is.read(byte_buffer, camera.MAX_LENGTH_FILE_PATH);
        camera._im_file_name = string(byte_buffer);
        camera._im_file_name = trim(camera._im_file_name);

        // if(DEBUG)
            printf("\nFile path: \'%s\'\n", camera._im_file_name.c_str());

        //        camera.read_metadata(is);

        return is;
    }

  private:
    void read_image()
    {
        FILE *fp = fopen(_im_file_name.c_str(), "rb");
        if(!fp)
        {
            std::stringstream sstr;
            sstr << "Can't open file: \'" << _im_file_name << '\'';
            throw std::runtime_error(sstr.str());
        }
        fseek(fp, 0, SEEK_END);
        size_t fsize = (size_t)ftell(fp);
        rewind(fp);
        unsigned char *buf = new unsigned char[fsize];
        if(fread(buf, 1, fsize, fp) != fsize)
        {
            delete[] buf;
            std::stringstream sstr;
            sstr << "Can't read file: \'" << _im_file_name << '\'';
            throw std::runtime_error(sstr.str());
        }
        fclose(fp);

        easyexif::EXIFInfo result;
        result.parseFrom(buf, (unsigned int)fsize);

        _focal_length = result.FocalLength * 0.001;
        _fp_resolution_x = result.LensInfo.FocalPlaneResolutionUnit == 2 ? result.LensInfo.FocalPlaneXResolution / 0.0254 : result.LensInfo.FocalPlaneXResolution / 0.01;
        _fp_resolution_y = result.LensInfo.FocalPlaneResolutionUnit == 2 ? result.LensInfo.FocalPlaneYResolution / 0.0254 : result.LensInfo.FocalPlaneYResolution / 0.01;

        // if(DEBUG)
            // printf("Focal length: %f, FP Resolution X: %f, Y: %f\n", _focal_length, _fp_resolution_x, _fp_resolution_y);
    }

    void calculate_frustum()
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

    void update_transformation()
    {
        
        scm::math::mat4f matrix_translation = scm::math::make_translation(scm::math::vec3f(_translation));
        std::cout << _translation << std::endl;
        // std::cout << _orientation << std::endl;
        scm::math::mat4f matrix_rotation = scm::math::mat4f(_orientation.to_matrix());
        _transformation = matrix_translation * matrix_rotation;
        // _still_image.update_transformation(_transformation, _scale);
    }

    uint16_t _index;
    double _focal_length;
    string _im_file_name;
    quatd _orientation;
    vec3d _translation;


    scm::math::mat4f _transformation = scm::math::mat4f::identity();
    scm::gl::vertex_array_ptr _vertex_array_object_frustum;


    int _im_height;
    int _im_width;
    double _fp_resolution_x;
    double _fp_resolution_y;
    arr<vec3d, 8> _frustum_vertices;
};
}

#endif // LAMURE_CAMERA_H
