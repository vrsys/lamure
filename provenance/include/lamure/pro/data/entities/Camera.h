#ifndef LAMURE_CAMERA_H
#define LAMURE_CAMERA_H

#include "MetaData.h"
#include "lamure/pro/common.h"

#include <memory>

#include <fstream>
#include <map>
#include <vector>

namespace prov
{
class Camera
{
  public:
    uint16_t MAX_LENGTH_FILE_PATH;

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
    }

    int get_index() { return _index; }
    quatd get_orientation() { return _orientation; }
    vec3d get_translation() { return _translation; }
    friend ifstream &operator>>(ifstream &is, Camera &camera)
    {
        is.read(reinterpret_cast<char *>(&camera._index), 2);
        camera._index = swap(camera._index, true);

        // if(DEBUG)
        // printf("\nIndex: %i", camera._index);

        is.read(reinterpret_cast<char *>(&camera._focal_length), 8);
        camera._focal_length = swap(camera._focal_length, true);

        // if(DEBUG)
        // printf("\nFocal length: %f", camera._focal_length);

        double w, x, y, z;
        is.read(reinterpret_cast<char *>(&w), 8);
        is.read(reinterpret_cast<char *>(&x), 8);
        is.read(reinterpret_cast<char *>(&y), 8);
        is.read(reinterpret_cast<char *>(&z), 8);
        w = swap(w, true);
        x = swap(x, true);
        y = swap(y, true);
        z = swap(z, true);

        // if(DEBUG)
        // printf("\nWXYZ: %f %f %f %f", w, x, y, z);

        quatd quat_tmp = quatd(w, x, y, z);
        scm::math::quat<double> new_orientation = scm::math::quat<double>::from_axis(180, scm::math::vec3d(1.0, 0.0, 0.0));
        quat_tmp = scm::math::normalize(quat_tmp);
        camera._orientation = scm::math::quat<double>::from_matrix(camera.SetQuaternionRotation(quat_tmp)) * new_orientation;
        // camera._orientation = quatd(w, x, y, z);

        is.read(reinterpret_cast<char *>(&x), 8);
        is.read(reinterpret_cast<char *>(&y), 8);
        is.read(reinterpret_cast<char *>(&z), 8);
        x = swap(x, true);
        y = swap(y, true);
        z = swap(z, true);

        // if(DEBUG)
        // printf("\nXYZ: %f %f %f", x, y, z);

        camera._translation = vec3d(x, y, z);

        char byte_buffer[camera.MAX_LENGTH_FILE_PATH];
        is.read(byte_buffer, camera.MAX_LENGTH_FILE_PATH);
        camera._im_file_name = string(byte_buffer);
        camera._im_file_name = trim(camera._im_file_name);

        // if(DEBUG)
        // printf("\nFile path: \'%s\'\n", camera._im_file_name.c_str());

        //        camera.read_metadata(is);

        return is;
    }

  private:
    scm::math::mat3d SetQuaternionRotation(const scm::math::quat<double> q)
    {
        scm::math::mat3d m = scm::math::mat3d::identity();
        double qw = q.w;
        double qx = q.i;
        double qy = q.j;
        double qz = q.k;
        m[0] = (qw * qw + qx * qx - qz * qz - qy * qy);
        m[1] = (2 * qx * qy - 2 * qz * qw);
        m[2] = (2 * qy * qw + 2 * qz * qx);
        m[3] = (2 * qx * qy + 2 * qw * qz);
        m[4] = (qy * qy + qw * qw - qz * qz - qx * qx);
        m[5] = (2 * qz * qy - 2 * qx * qw);
        m[6] = (2 * qx * qz - 2 * qy * qw);
        m[7] = (2 * qy * qz + 2 * qw * qx);
        m[8] = (qz * qz + qw * qw - qy * qy - qx * qx);
        return m;
    }

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

        _im_height = result.ImageHeight;
        _im_width = result.ImageWidth;
        _focal_length = result.FocalLength * 0.001;
        _fp_resolution_x = result.LensInfo.FocalPlaneResolutionUnit == 2 ? result.LensInfo.FocalPlaneXResolution / 0.0254 : result.LensInfo.FocalPlaneXResolution / 0.01;
        _fp_resolution_y = result.LensInfo.FocalPlaneResolutionUnit == 2 ? result.LensInfo.FocalPlaneYResolution / 0.0254 : result.LensInfo.FocalPlaneYResolution / 0.01;

        if(_fp_resolution_x == 0 && _fp_resolution_y == 0)
        {
            _fp_resolution_x = 5715.545755 / 0.0254;
            _fp_resolution_y = 5808.403361 / 0.0254;
        }

        // if(DEBUG)
        // printf("Focal length: %f, FP Resolution X: %f, Y: %f\n", _focal_length, _fp_resolution_x, _fp_resolution_y);
    }

  protected:
    uint16_t _index;
    double _focal_length;
    string _im_file_name;
    quatd _orientation;
    vec3d _translation;

    int _im_height;
    int _im_width;
    double _fp_resolution_x;
    double _fp_resolution_y;
};
}

#endif // LAMURE_CAMERA_H
