#version 440 core

out vec2 texture_coord;

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texture_coord;

void main()
{
    texture_coord = in_texture_coord;
    gl_Position = projection_matrix * model_view_matrix * (vec4(in_position, 1.0));
}