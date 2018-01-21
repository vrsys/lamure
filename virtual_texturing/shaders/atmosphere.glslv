#version 440 core

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;

layout(location = 0) in vec3 in_position;

void main()
{
    gl_Position = projection_matrix * model_view_matrix * vec4(in_position, 1.0);
}