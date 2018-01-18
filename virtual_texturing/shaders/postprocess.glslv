#version 440

uniform mat4 mvp;

layout(location = 0) in vec3 in_position;

void main()
{
    gl_Position = mvp * vec4(in_position, 1.0);
}