#version 440 core

out vec2 texture_coord;
flat out uvec2 physical_texture_dim;
flat out uvec2 index_texture_dim;
flat out uint max_level;
flat out uint toggle_view;

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;
uniform uint in_max_level;
uniform int in_toggle_view;
uniform uvec2 in_physical_texture_dim;
uniform uvec2 in_index_texture_dim;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texture_coord;

void main()
{
    max_level = in_max_level;
    toggle_view = in_toggle_view;
    texture_coord = in_texture_coord;
    index_texture_dim = in_index_texture_dim;
    physical_texture_dim = in_physical_texture_dim;

    // gl_Position = /*projection_matrix * model_view_matrix **/ vec4(in_position, 1.0);
    gl_Position = projection_matrix * model_view_matrix * vec4(in_position, 1.0);
}