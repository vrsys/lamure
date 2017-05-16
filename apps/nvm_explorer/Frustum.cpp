#include "Frustum.h"

Frustum::Frustum()
{
}
void Frustum::init(scm::shared_ptr<scm::gl::render_device> device, std::vector<scm::math::vec3f> vertices)
{
    _near_left_top = vertices[0];
    _near_right_top = vertices[1];
    _near_left_bottom = vertices[2];
    _near_right_bottom = vertices[3];
    _far_left_top = vertices[4];
    _far_right_top = vertices[5];
    _far_left_bottom = vertices[6];
    _far_right_bottom = vertices[7];
    
    std::vector<Struct_Line> vector_struct_lines = convert_frustum_to_struct_line();
    // Struct_Line struct_line = {scm::math::vec3f(1.0f, 1.0f, 1.0f)};
    // vector_struct_lines.push_back(struct_line);
    // Struct_Line struct_line1 = {scm::math::vec3f(3.0f, 3.0f, 1.0f)};
    // vector_struct_lines.push_back(struct_line1);

    scm::gl::buffer_ptr vertex_buffer_object = device->create_buffer(scm::gl::BIND_VERTEX_BUFFER,
                                    scm::gl::USAGE_STATIC_DRAW,
                                    (sizeof(float) * 3) * vector_struct_lines.size(),
                                    &vector_struct_lines[0]);

    _vertex_array_object = device->create_vertex_array(scm::gl::vertex_format
        (0, 0, scm::gl::TYPE_VEC3F, sizeof(float) * 3),
        boost::assign::list_of(vertex_buffer_object));


    // device->main_context()->apply();
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
scm::gl::vertex_array_ptr Frustum::get_vertex_array_object()
{
    return _vertex_array_object;
}
