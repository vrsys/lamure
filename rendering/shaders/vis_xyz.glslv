// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 420 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in float in_r;
layout(location = 2) in float in_g;
layout(location = 3) in float in_b;
layout(location = 4) in float empty;
layout(location = 5) in float in_radius;
layout(location = 6) in vec3 in_normal;

layout(location = 7) in float prov_float;
layout(location = 8) in float prov_float1;
layout(location = 9) in float prov_float2;
layout(location = 10) in float prov_float3;
layout(location = 11) in float prov_float4;
layout(location = 12) in float prov_float5;


uniform mat4 inv_mv_matrix;
uniform float model_radius_scale;
uniform float point_size_factor;

uniform float heatmap_min;
uniform float heatmap_max;
uniform vec3 heatmap_min_color;
uniform vec3 heatmap_max_color;

out VertexData {
  //output to geometry shader
  vec3 pass_ms_u;
  vec3 pass_ms_v;

  vec3 pass_point_color;
  vec3 pass_normal;
} VertexOut;

vec3 quick_interp(vec3 color1, vec3 color2, float value) {
  return color1 + (color2 - color1) * clamp(value, 0, 1);
}

void main()
{
  vec3 ms_n = normalize(in_normal.xyz);
  vec3 ms_u;

  //compute tangent vectors
  if(ms_n.z != 0.0) {
    ms_u = vec3( 1, 1, (-ms_n.x -ms_n.y)/ms_n.z);
  } else if (ms_n.y != 0.0) {
    ms_u = vec3( 1, (-ms_n.x -ms_n.z)/ms_n.y, 1);
  } else {
    ms_u = vec3( (-ms_n.y -ms_n.z)/ms_n.x, 1, 1);
  }

  //assign tangent vectors
  VertexOut.pass_ms_u = normalize(ms_u) * point_size_factor * model_radius_scale * in_radius;
  VertexOut.pass_ms_v = normalize(cross(ms_n, ms_u)) * point_size_factor * model_radius_scale * in_radius;

  VertexOut.pass_normal = normalize((inv_mv_matrix * vec4(in_normal.xyz, 0.0)).xyz );

  gl_Position = vec4(in_position.xyz, 1.0);

  //VertexOut.pass_point_color = vec3(in_r, in_g, in_b);

  float value = (prov_float1 - heatmap_min) / (heatmap_max - heatmap_min);
  VertexOut.pass_point_color = quick_interp(heatmap_min_color, heatmap_max_color, value);

}

