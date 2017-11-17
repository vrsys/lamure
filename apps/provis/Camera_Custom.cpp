#include "Camera_Custom.h"

Camera_Custom::Camera_Custom(prov::Camera camera) : prov::Camera(camera)
{
    update_transformation();
    // std::cout << _fp_resolution_x << std::endl;
    float width_world = (1.0f / _fp_resolution_x) * _im_width;
    float height_world = (1.0f / _fp_resolution_y) * _im_height;

    _frustum = Frustum(width_world, height_world, _focal_length);
}

void Camera_Custom::update_transformation_image_plane()
{
    scm::math::mat4f matrix_translation = scm::math::make_translation(scm::math::vec3f(0.0, 0.0, -_focal_length));
    // // normalize quad to 1m*1m
    float scale = 0.5;
    // // get the width of quad in world dimension (should be sensor size)
    scale *= (1.0f / _fp_resolution_x) * _im_width;
    // // increase size for better visibility
    // scale *= _frustum.get_scale();

    float aspect_ratio = float(_im_height) / _im_width;
    scm::math::mat4f matrix_scale = scm::math::make_scale(scm::math::vec3f(scale, scale * aspect_ratio, scale));
    // _transformation_image_plane = _transformation * matrix_translation;
    _transformation_image_plane = _transformation * matrix_translation * matrix_scale;
}
void Camera_Custom::update_pixels_brush()
{
    // _vector_pixels_brush.push_back(position_pixel);

    // _vertex_buffer_object_pixels = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 3) * _vector_pixels_brush.size(), &_vector_pixels_brush[0]);
    // _vertex_array_object_pixels = device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3), boost::assign::list_of(_vertex_buffer_object_pixels));
}

void Camera_Custom::reset_pixels_brush(scm::shared_ptr<scm::gl::render_device> device)
{
    _vector_pixels_brush.clear();
    _vector_pixels_not_seen_brush.clear();

    _vertex_buffer_object_pixels = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 3) * _vector_pixels_brush.size(), &_vector_pixels_brush[0]);
    _vertex_array_object_pixels = device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3), boost::assign::list_of(_vertex_buffer_object_pixels));

    _vertex_buffer_object_pixels_not_seen =
        device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 3) * _vector_pixels_not_seen_brush.size(), &_vector_pixels_not_seen_brush[0]);
    _vertex_array_object_pixels_not_seen =
        device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3), boost::assign::list_of(_vertex_buffer_object_pixels_not_seen));
}

void Camera_Custom::add_pixel_brush(scm::math::vec3f position_surfel_brush, scm::shared_ptr<scm::gl::render_device> device, bool seen)
{
    // direction of ray
    scm::math::vec3f direction_normalized = scm::math::normalize(position_surfel_brush - _translation);

    // viewing direction of camera
    scm::math::vec3f direction_viewing_normalized = -scm::math::normalize(scm::math::vec3f(_transformation[8], _transformation[9], _transformation[10]));

    float dot = scm::math::dot(direction_normalized, direction_viewing_normalized);
    float angle_between_directions = scm::math::rad2deg(scm::math::acos(dot));

    float angle_c = 90.0f - angle_between_directions;

    float length = (_focal_length / scm::math::sin(scm::math::deg2rad(angle_c))) * scm::math::sin(scm::math::deg2rad(90.0f));

    scm::math::vec3f position_pixel = scm::math::vec3f(_translation) + length * _frustum.get_scale() * direction_normalized;

    if(seen == true)
    {
        _vector_pixels_brush.push_back(scm::math::inverse(_transformation_image_plane) * position_pixel);
        _vertex_buffer_object_pixels = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 3) * _vector_pixels_brush.size(), &_vector_pixels_brush[0]);
        _vertex_array_object_pixels = device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3), boost::assign::list_of(_vertex_buffer_object_pixels));
    }
    else
    {
        _vector_pixels_not_seen_brush.push_back(scm::math::inverse(_transformation_image_plane) * position_pixel);
        _vertex_buffer_object_pixels_not_seen =
            device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 3) * _vector_pixels_not_seen_brush.size(), &_vector_pixels_not_seen_brush[0]);
        _vertex_array_object_pixels_not_seen =
            device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3), boost::assign::list_of(_vertex_buffer_object_pixels_not_seen));
    }
}

// void Camera_Custom::add_pixel_brush(scm::math::vec3f position_surfel_brush, scm::shared_ptr<scm::gl::render_device> device)
// {
//     // direction of ray
//     scm::math::vec3f direction_normalized = scm::math::normalize(position_surfel_brush - _translation);

//     // viewing direction of camera
//     scm::math::vec3f direction_viewing_normalized = -scm::math::normalize(scm::math::vec3f(_transformation[8], _transformation[9], _transformation[10]));

//     float dot = scm::math::dot(direction_normalized, direction_viewing_normalized);
//     float angle_between_directions = scm::math::rad2deg(scm::math::acos(dot));

//     float angle_c = 90.0f - angle_between_directions;

//     float length = (_focal_length / scm::math::sin(scm::math::deg2rad(angle_c))) * scm::math::sin(scm::math::deg2rad(90.0f));

//     scm::math::vec3f position_pixel = scm::math::vec3f(_translation) + length * _frustum.get_scale() * direction_normalized;

//     _vector_pixels_brush.push_back(position_pixel);

//     _vertex_buffer_object_pixels = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 3) * _vector_pixels_brush.size(), &_vector_pixels_brush[0]);
//     _vertex_array_object_pixels = device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3), boost::assign::list_of(_vertex_buffer_object_pixels));
// }

std::vector<Struct_Line> Camera_Custom::convert_lines_to_struct_line(std::vector<prov::SparsePoint> &vector_point)
{
    std::vector<Struct_Line> vector_points;
    for(prov::SparsePoint &point_sparse : vector_point)
    {
        for(prov::SparsePoint::Measurement measurement : point_sparse.get_measurements())
        {
            if(measurement.get_camera() == _index)
            {
                Struct_Line position_point = {scm::math::vec3f(point_sparse.get_position())};
                vector_points.push_back(position_point);
            }
        }
    }

    _count_lines = vector_points.size();

    std::random_shuffle(vector_points.begin(), vector_points.end());

    Struct_Line position_camera = {scm::math::vec3f(_translation)};

    std::vector<Struct_Line> vector_struct_line;
    for(Struct_Line &point : vector_points)
    {
        vector_struct_line.push_back(position_camera);
        vector_struct_line.push_back(point);
    }

    // for(prov::SparsePoint sparsePoint it = _vector_point.begin(); it != _vector_point.end(); ++it)
    //     for(std::vector<prov::SparsePoint>::iterator it = _vector_point.begin(); it != _vector_point.end(); ++it)
    //     {
    //         prov::SparsePoint &point = (*it);
    //         // std::vector<prov::SparsePoint::measurement> measurements = point.get_measurements();
    //     }

    // for(std::vector<prov::Camera>::iterator it = _vector_camera.begin(); it != _vector_camera.end(); ++it)
    // {
    //     Struct_Line line = {scm::math::vec3f((*it).get_translation())};
    //     vector_struct_line.push_back(line);
    //     // Struct_Line line1 = {scm::math::vec3f((*it).get_translation())+scm::math::vec3f(10.0, 10.0, 10.0)};
    //     line = {scm::math::vec3f(0.0, 0.0, 0.0)};
    //     vector_struct_line.push_back(line);
    //     // vector_struct_line.push_back(line);
    //     // vector_struct_line.push_back(line);
    //     // vector_struct_line.push_back(line);
    //     // vector_struct_line.push_back(line);
    //     // std::cout << (*it).get_translation() << std::endl;
    //     break;
    // }
    return vector_struct_line;
}

// scm::gl::sampler_state_ptr Camera_Custom::get_state() { return _state; }

void Camera_Custom::load_texture(scm::shared_ptr<scm::gl::render_device> device)
{
    scm::gl::texture_loader tl;
    std::cout << "creating texture " << _im_file_name << ", camera index:" << _index << std::endl;
    // _texture.reset();
    _texture = tl.load_texture_2d(*device, _im_file_name, true, false);

    _state = device->create_sampler_state(scm::gl::FILTER_MIN_MAG_LINEAR, scm::gl::WRAP_CLAMP_TO_EDGE);
}
// scm::gl::texture_2d_ptr Camera_Custom::get_texture() { return _texture; }
scm::gl::vertex_array_ptr Camera_Custom::get_vertex_array_object_lines() { return _vertex_array_object_lines; }
scm::gl::vertex_array_ptr Camera_Custom::get_vertex_array_object_pixels() { return _vertex_array_object_pixels; }
scm::gl::vertex_array_ptr Camera_Custom::get_vertex_array_object_pixels_not_seen() { return _vertex_array_object_pixels_not_seen; }

void Camera_Custom::update_transformation()
{
    scm::math::mat4f matrix_translation = scm::math::make_translation(scm::math::vec3f(_translation));
    scm::math::mat4f matrix_rotation = scm::math::mat4f(_orientation.to_matrix());
    scm::math::mat4f matrix_scale = scm::math::make_scale(scm::math::vec3f(_frustum.get_scale(), _frustum.get_scale(), _frustum.get_scale()));

    _transformation = matrix_translation * matrix_rotation * matrix_scale;
    update_transformation_image_plane();
}

void Camera_Custom::update_scale_frustum(float offset)
{
    _frustum.update_scale_frustum(offset);
    update_transformation();
    update_pixels_brush();
}

int &Camera_Custom::get_count_lines() { return _count_lines; }
scm::math::mat4f &Camera_Custom::get_transformation() { return _transformation; }
scm::math::mat4f &Camera_Custom::get_transformation_image_plane() { return _transformation_image_plane; }

// const Image &Camera_Custom::get_still_image() const { return _still_image; }

void Camera_Custom::init(scm::shared_ptr<scm::gl::render_device> device, std::vector<prov::SparsePoint> &vector_point)
{
    // create buffer for the lines connecting the sparse points with the projection center
    std::vector<Struct_Line> vector_struct_line = convert_lines_to_struct_line(vector_point);
    _vertex_buffer_object_lines = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 3) * vector_struct_line.size(), &vector_struct_line[0]);
    _vertex_array_object_lines = device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3), boost::assign::list_of(_vertex_buffer_object_lines));

    load_texture(device);
    _frustum.init(device);
}

std::vector<scm::math::vec3f> &Camera_Custom::get_vector_pixels_brush() { return _vector_pixels_brush; }
std::vector<scm::math::vec3f> &Camera_Custom::get_vector_pixels_not_seen_brush() { return _vector_pixels_not_seen_brush; }

void Camera_Custom::bind_texture(scm::shared_ptr<scm::gl::render_context> context) { context->bind_texture(_texture, _state, 0); }

Frustum &Camera_Custom::get_frustum() { return _frustum; }