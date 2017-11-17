#include "Frustum.h"

Frustum::Frustum() {}
Frustum::Frustum(float image_width_world, float image_height_world, double focal_length) : _image_height_world(image_height_world), _image_width_world(image_width_world), _focal_length(focal_length)
{
    calc_frustum_points();
}

void Frustum::init(scm::shared_ptr<scm::gl::render_device> device)
{
    std::vector<Struct_Line> vector_struct_lines = convert_frustum_to_struct_line();

    // std::cout << vector_struct_lines[0].position << std::endl;
    // std::cout << vector_struct_lines[1].position << std::endl;
    // std::cout << vector_struct_lines[2].position << std::endl;
    // std::cout << vector_struct_lines[3].position << std::endl;
    // std::cout << vector_struct_lines[4].position << std::endl;
    // std::cout << vector_struct_lines[5].position << std::endl;
    // std::cout << vector_struct_lines[6].position << std::endl;
    // std::cout << vector_struct_lines[7].position << std::endl;

    _vertex_buffer_object = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER, scm::gl::USAGE_STATIC_DRAW, (sizeof(float) * 3) * vector_struct_lines.size(), &vector_struct_lines[0]);
    _vertex_array_object = device->create_vertex_array(scm::gl::vertex_format(0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3), boost::assign::list_of(_vertex_buffer_object));

    // device->main_context()->apply();
}
void Frustum::update_scale_frustum(float offset)
{
    float new_scale = std::max(_scale + offset, 0.0f);
    _scale = new_scale;
    // calc_frustum_points();
    // init()
    // update_frustum_points();
    // update_transformation();
    // _frustum.init(device, calc_frustum_points());
}

void Frustum::calc_frustum_points()
{
    float width_world_half = _image_width_world / 2;
    float height_world_half = _image_height_world / 2;

    // width_world_half *= _scale;
    // height_world_half *= _scale;

    _near_left_top = scm::math::vec3f(0.0, 0.0, 0.0);
    _near_right_top = scm::math::vec3f(0.0, 0.0, 0.0);
    _near_left_bottom = scm::math::vec3f(0.0, 0.0, 0.0);
    _near_right_bottom = scm::math::vec3f(0.0, 0.0, 0.0);
    _far_left_top = scm::math::vec3f(-width_world_half, height_world_half, -_focal_length);
    _far_right_top = scm::math::vec3f(width_world_half, height_world_half, -_focal_length);
    _far_left_bottom = scm::math::vec3f(-width_world_half, -height_world_half, -_focal_length);
    _far_right_bottom = scm::math::vec3f(width_world_half, -height_world_half, -_focal_length);
    // scm::math::vec3d _direction_of_camera = scm::math::vec3d(0.0, 0.0, -1.0);
    // scm::math::vec3d center_image_plane = _center + _focal_length * scm::math::normalize(_direction_of_camera);
    // width_image_plane = (1.0f/_still_image.get_fp_resolution_x()) * _still_image.get_width();
}

std::vector<Struct_Line> Frustum::convert_frustum_to_struct_line()
{
    std::vector<Struct_Line> vector_struct_line;

    vector_struct_line.push_back({_near_left_top});
    vector_struct_line.push_back({_near_right_top});

    vector_struct_line.push_back({_near_right_top});
    vector_struct_line.push_back({_near_right_bottom});

    vector_struct_line.push_back({_near_right_bottom});
    vector_struct_line.push_back({_near_left_bottom});

    vector_struct_line.push_back({_near_left_bottom});
    vector_struct_line.push_back({_near_left_top});

    vector_struct_line.push_back({_far_left_top});
    vector_struct_line.push_back({_far_right_top});

    vector_struct_line.push_back({_far_right_top});
    vector_struct_line.push_back({_far_right_bottom});

    vector_struct_line.push_back({_far_right_bottom});
    vector_struct_line.push_back({_far_left_bottom});

    vector_struct_line.push_back({_far_left_bottom});
    vector_struct_line.push_back({_far_left_top});

    vector_struct_line.push_back({_near_left_top});
    vector_struct_line.push_back({_far_left_top});

    vector_struct_line.push_back({_near_right_top});
    vector_struct_line.push_back({_far_right_top});

    vector_struct_line.push_back({_near_right_bottom});
    vector_struct_line.push_back({_far_right_bottom});

    vector_struct_line.push_back({_near_left_bottom});
    vector_struct_line.push_back({_far_left_bottom});

    // scm::math::vec3f color_normalized = scm::math::vec3f((*it).get_color()) / 255.0;
    // Struct_Line point = {(*it).get_center(), color_normalized};

    // vector_struct_line.push_back(point);

    return vector_struct_line;
}
scm::gl::vertex_array_ptr const &Frustum::get_vertex_array_object() { return _vertex_array_object; }
float &Frustum::get_scale() { return _scale; }
