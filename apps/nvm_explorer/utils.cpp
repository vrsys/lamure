#include "utils.h"
#include <boost/filesystem.hpp>

namespace utils {

    template<typename T>
    vec<T, 2> pair_to_vec2(T v0, T v1) {
        return vec<T, 2>(v0, v1);
    }

    template<typename T>
    vec<T, 3> arr3_to_vec3(T *arr) {
        return vec<T, 3>(arr[0], arr[1], arr[2]);
    }

    template<typename T>
    mat<T, 3, 3> arr9_to_mat3(T *arr) {
        return mat<T, 3, 3>(arr[0], arr[1], arr[2],
                            arr[3], arr[4], arr[5],
                            arr[6], arr[7], arr[8]);
    };

    // function definition in accordance with Util.h of VisualSfM pipeline
    bool read_nmv(ifstream &in,
                          vector<camera> &camera_vec,
                          vector<point> &point_vec,
                          vector<image> &images) {
        unsigned int rotation_parameter_num = 4;
        bool format_r9t = false;
        string token;
        if (in.peek() == 'N') {
            in >> token;  // file header
            if (strstr(token.c_str(), "R9T")) {
                rotation_parameter_num = 9;  // rotation as 3x3 matrix
                format_r9t = true;
            }
            getline(in, token);
        }

        unsigned int nproj = 0;
        unsigned long npoint = 0;
        unsigned long ncam = 0;
        // read # of cameras
        in >> ncam;
        if (ncam <= 1) return false;

        // read the camera parameters
        camera_vec.resize(ncam);
        images.resize(ncam);
        for (unsigned int i = 0; i < ncam; ++i) {
            double f, q[9], c[3], d[2];
            in >> token >> f;
            for (unsigned int j = 0; j < rotation_parameter_num; ++j) in >> q[j];
            in >> c[0] >> c[1] >> c[2] >> d[0] >> d[1];

            camera_vec[i].set_focal_length(f);
            if (format_r9t) {
                // matrix input
                camera_vec[i].set_orientation(quat<double>::from_matrix(arr9_to_mat3(q)));
                camera_vec[i].set_center(arr3_to_vec3(c));
            } else {
                // quaternion input
                camera_vec[i].set_orientation(quat<double>(q[0], q[1], q[3], q[4]));
                camera_vec[i].set_center(arr3_to_vec3(c));
            }
            camera_vec[i].set_radial_distortion(d[0]);
            images[i] = image::read_from_file(token);
            camera_vec[i].set_still_image(images[i]);
        }

        in >> npoint;
        if (npoint <= 0) return false;

        // read image projections and 3D points.
        point_vec.resize(npoint);
        for (unsigned int i = 0; i < npoint; ++i) {
            float pt[3];
            int cc[3];
            unsigned int npj;
            in >> pt[0] >> pt[1] >> pt[2] >> cc[0] >> cc[1] >> cc[2] >> npj;
            point_vec[i].set_center(arr3_to_vec3(pt));
            point_vec[i].set_color(arr3_to_vec3(cc));
            for (unsigned int j = 0; j < npj; ++j) {
                int cidx, fidx;
                float imx, imy;
                in >> cidx >> fidx >> imx >> imy;

                vector<point::measurement> measurements = point_vec[i].get_measurements();
                measurements.push_back(point::measurement(camera_vec[cidx].get_still_image(),pair_to_vec2(imx, imy)));
                nproj++;
            }
        }

        std::cout << ncam << " cameras; " << npoint << " 3D points; " << nproj << " projections\n";

        return true;
    }

}