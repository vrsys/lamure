// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

layout (location = 0) in vec4 vertex_position;
layout (location = 1) in vec4 vertex_normal;
layout (location = 2) in vec2 vertex_coord;

uniform mat4 mvp_matrix;

out vertex_data {
    vec4 position;
    vec4 normal;
    vec2 coord;
} vertex_out;


void main() {
  gl_Position = mvp_matrix * vertex_position;
}
