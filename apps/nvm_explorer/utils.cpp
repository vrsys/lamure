#include "utils.h"

namespace utils
{
template <typename T>
vec<T, 2> pair_to_vec2(T v0, T v1)
{
    return vec<T, 2>(v0, v1);
}

template <typename T>
vec<T, 3> arr3_to_vec3(T *arr)
{
    return vec<T, 3>(arr[0], arr[1], arr[2]);
}

template <typename T>
mat<T, 3, 3> arr9_to_mat3(T *arr)
{
    return mat<T, 3, 3>(arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7], arr[8]);
};

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
// mat<scal_type, 4, 4> ret = mat<scal_type, 4, 4>::zero();

//  ret.data_array[0 * 4 + 0] = scal_type(1) - scal_type(2) * y * y - scal_type(2) * z * z;
//  ret.data_array[1 * 4 + 0] = scal_type(2) * x * y - scal_type(2) * w * z;
//  ret.data_array[2 * 4 + 0] = scal_type(2) * x * z + scal_type(2) * w * y;

//  ret.data_array[0 * 4 + 1] = scal_type(2) * x * y + scal_type(2) * w * z;
//  ret.data_array[1 * 4 + 1] = scal_type(1) - scal_type(2) * x * x - scal_type(2) * z * z;
//  ret.data_array[2 * 4 + 1] = scal_type(2) * y * z - scal_type(2) * w * x;

//  ret.data_array[0 * 4 + 2] = scal_type(2) * x * z - scal_type(2) * w * y;
//  ret.data_array[1 * 4 + 2] = scal_type(2) * y * z + scal_type(2) * w * x;
//  ret.data_array[2 * 4 + 2] = scal_type(1) - scal_type(2) * x * x - scal_type(2) * y * y;

//  ret.data_array[3 * 4 + 3] = scal_type(1);

// function definition in accordance with Util.h of VisualSfM pipeline
bool read_nvm(ifstream &in, vector<Camera> &camera_vec, vector<Point> &point_vec, vector<Image> &images)
{
    unsigned int rotation_parameter_num = 4;
    bool format_r9t = false;
    string token;
    if(in.peek() == 'N')
    {
        in >> token; // file header
        if(strstr(token.c_str(), "R9T"))
        {
            rotation_parameter_num = 9; // rotation as 3x3 matrix
            format_r9t = true;
        }
        getline(in, token);
    }

    unsigned int nproj = 0;
    unsigned long npoint = 0;
    unsigned long ncam = 0;
    // read # of cameras
    in >> ncam;
    if(ncam <= 1)
        return false;

    // read the Camera parameters
    camera_vec.resize(ncam);
    images.resize(ncam);
    for(unsigned int i = 0; i < ncam; ++i)
    {
        double f, q[9], c[3], d[2];
        in >> token >> f;
        for(unsigned int j = 0; j < rotation_parameter_num; ++j)
            in >> q[j];
        in >> c[0] >> c[1] >> c[2] >> d[0] >> d[1];

        camera_vec[i].set_focal_length(f);
        if(format_r9t)
        {
            // matrix input
            camera_vec[i].set_orientation(quat<double>::from_matrix(arr9_to_mat3(q)));
            camera_vec[i].set_center(arr3_to_vec3(c));
        }
        else
        {
            // quaternion input

            scm::math::quat<double> quat = scm::math::quat<double>(q[0], q[1], q[2], q[3]);
            // scm::math::vec3d axis;
            // double angle;
            // quat.retrieve_axis_angle(angle, axis);

            // scm::math::quat<double> quat_rot = scm::math::quat<double>::from_axis(180+angle, axis);

            // scm::math::quat<double> quat = scm::math::quat<double>::from_axis(180.0, scm::math::vec3d(0.0, 1.0, 0.0));
            // scm::math::quat<double> quat = scm::math::quat<double>::from_axis(0.0, scm::math::vec3d(0.0, 1.0, 0.0));
            // scm::math::quat<double> quat = scm::math::quat<double> (-0.979892074788, -0.127325981533, 0.150984070102, -0.0283534293648);
            // scm::math::quat<double> quat = scm::math::quat<double> (0.979892074788, -0.127325981533, 0.150984070102, -0.0283534293648);
            // scm::math::quat<double> quat = scm::math::quat<double> (0.979892074788, 0.127325981533, -0.150984070102, 0.0283534293648);
            // scm::math::quat<double> quat = scm::math::quat<double> (-0.979892074788, 0.127325981533, -0.150984070102, 0.0283534293648);
            // scm::math::quat<double> quat = scm::math::quat<double> (-0.979892074788, 0.127325981533, -0.150984070102, 0.0283534293648);
            // scm::math::quat<double> quat = scm::math::quat<double> (1.0, 0.0, 0.0, 0.0);
            // scm::math::quat<double> quat = scm::math::quat<double> (3.1685, 3.7559, -0.7056, 0.4955);
            // quat = quat * quat_rot;
            // std::cout << quat << std::endl;
            // quat = scm::math::normalize(quat);
            // std::cout << quat << std::endl;
            // quat = scm::math::conjugate(quat);
            // std::cout << quat << std::endl;
            // scm::math::mat4d matrix_rotation = quat.to_matrix();
            // std::cout << matrix_rotation[2]<<" " << matrix_rotation[6]<< " " << matrix_rotation[10] << std::endl;
            // std::cout << M_PI+angle <<" " << angle << " " << axis << std::endl;
            // quat = quat * (1.0 / scm::math::normalize(quat));
            // std::cout << quat << std::endl;
            // quat = scm::math::quat<double>::from_axis(180.0, scm::math::vec3d(0.0, 1.0, 0.0));

            // std::cout << quat << std::endl;
            // conjugate().scale(1/norm());

            // quat *= scm::math::quat<double>::from_axis(180.0, scm::math::vec3d(0.0, 1.0, 0.0));

            // scm::math::quat<double> old_orientation = camera.get_orientation();
            scm::math::quat<double> new_orientation = scm::math::quat<double>::from_axis(180, scm::math::vec3d(1.0, 0.0, 0.0));
            quat = scm::math::normalize(quat);
            // ;
            // camera_vec[i].set_orientation(scm::math::quat<double>::from_matrix(quat.to_matrix()));
            camera_vec[i].set_orientation(scm::math::quat<double>::from_matrix(SetQuaternionRotation(quat)) * new_orientation);
            // camera_vec[i].set_orientation(scm::math::quat<double>::from_matrix(scm::math::inverse(new_orientation.to_matrix()) * quat.to_matrix() * new_orientation.to_matrix()));
            camera_vec[i].set_center(arr3_to_vec3(c));
        }
        camera_vec[i].set_radial_distortion(d[0]);

        images[i] = Image::read_from_file(token);

        camera_vec[i].set_still_image(images[i]);
    }

    in >> npoint;
    if(npoint <= 0)
        return false;

    // read Image projections and 3D points.
    point_vec.resize(npoint);
    for(unsigned int i = 0; i < npoint; ++i)
    {
        float pt[3];
        int cc[3];
        unsigned int npj;
        in >> pt[0] >> pt[1] >> pt[2] >> cc[0] >> cc[1] >> cc[2] >> npj;
        point_vec[i].set_center(arr3_to_vec3(pt));
        point_vec[i].set_color(arr3_to_vec3(cc));
        for(unsigned int j = 0; j < npj; ++j)
        {
            int cidx, fidx;
            float imx, imy;
            in >> cidx >> fidx >> imx >> imy;

            vector<Point::measurement> measurements = point_vec[i].get_measurements();
            measurements.push_back(Point::measurement(camera_vec[cidx].get_still_image(), pair_to_vec2(imx, imy)));
            point_vec[i].set_measurements(measurements);
            nproj++;
        }
    }

    std::cout << ncam << " cameras; " << npoint << " 3D points; " << nproj << " projections\n";

    return true;
}

bool read_ply(ifstream &in, vector<Point> &point_vec)
{
    tinyply::PlyFile file(in);

    for(tinyply::PlyElement e : file.get_elements())
    {
        std::cout << "element - " << e.name << " (" << e.size << ")" << std::endl;
        for(auto p : e.properties)
        {
            std::cout << "\tproperty - " << p.name << " (" << tinyply::PropertyTable[p.propertyType].str << ")" << std::endl;
        }
    }
    std::cout << std::endl;

    for(string c : file.comments)
    {
        std::cout << "Comment: " << c << std::endl;
    }

    std::vector<float> verts;
    std::vector<float> norms;
    std::vector<uint8_t> colors;

    file.request_properties_from_element("vertex", {"x", "y", "z"}, verts);
    file.request_properties_from_element("vertex", {"nx", "ny", "nz"}, norms);
    file.request_properties_from_element("vertex", {"diffuse_red", "diffuse_green", "diffuse_blue"}, colors);

    file.read(in);

    point_vec.resize(verts.size() / 3);

    for(int i = 0; i < verts.size() / 3; i++)
    {
        point_vec[i].set_center(vec3f(verts[i], verts[i + 1], verts[i + 2]));
        point_vec[i].set_color(vec3i(colors[i], colors[i + 1], colors[i + 2]));
        // TODO: set normals
        // std::cout << "Color: " << point_vec[i].get_color() << " Center: " << point_vec[i].get_center() << std::endl;
    }

    return true;
}
}