// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 420 core

layout (location = 0) out vec4 out_color;


uniform mat4 model_view_matrix;

in vertex_data {
    vec4 position;
    vec4 normal;
    vec2 coord;
} vertex_in;

OPTIONAL_BEGIN
  INCLUDE ../common/shading/blinn_phong.glsl
OPTIONAL_END

void main() {

  vec4 n = vertex_in.normal;
  vec3 nv = normalize((n*inverse(model_view_matrix)).xyz);


  //vec3 color = vec3(nv.x*0.5+0.5, nv.y*0.5+0.5, nv.z*0.5+0.5);
  //to test charting
  vec3 color = vec3(vertex_in.coord.x, vertex_in.coord.y, 0.5);

  OPTIONAL_BEGIN
    vec4 pos_es = model_view_matrix * vec4(vertex_in.position.xyz, 1.0f);
    color = shade_blinn_phong(pos_es.xyz, n.xyz, vec3(0.0, 0.0, 0.0), color);
  OPTIONAL_END

  out_color = vec4(color, 1.0);

}

