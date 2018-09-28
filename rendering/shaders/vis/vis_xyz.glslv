// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 420 core


out VertexData {
    //output to geometry shader
    vec3 pass_ms_u;
    vec3 pass_ms_v;

    vec3 pass_point_color;
    vec3 pass_normal;
    OPTIONAL_BEGIN
      vec3 mv_vertex_position;
    OPTIONAL_END
} VertexOut;

uniform mat4 mvp_matrix;
uniform mat4 model_matrix;
uniform mat4 model_view_matrix;
uniform mat4 inv_mv_matrix;
uniform mat4 model_to_screen_matrix;

uniform bool face_eye;
uniform vec3 eye;

uniform float near_plane;
uniform float max_radius;

uniform float point_size_factor;
uniform float model_radius_scale;


layout(location = 0) in vec3 in_position;
layout(location = 1) in float in_r;
layout(location = 2) in float in_g;
layout(location = 3) in float in_b;
layout(location = 4) in float empty;
layout(location = 5) in float in_radius;
layout(location = 6) in vec3 in_normal;

layout(location = 7) in float prov1;
layout(location = 8) in float prov2;
layout(location = 9) in float prov3;
layout(location = 10) in float prov4;
layout(location = 11) in float prov5;
layout(location = 12) in float prov6;


INCLUDE vis_color.glsl

void main()
{
  float radius = in_radius;
  if (radius > max_radius) {
    radius = max_radius;
  }

  vec3 normal = in_normal;
  if (face_eye) {
    normal = normalize(eye-(model_matrix*vec4(in_position, 1.0)).xyz);
  }
 
  // precalculate tangent vectors to establish the surfel shape
  vec3 tangent   = vec3(0.0);
  vec3 bitangent = vec3(0.0);
  compute_tangent_vectors(normal, radius, tangent, bitangent);

  if (!face_eye) {
    normal = normalize((inv_mv_matrix * vec4(in_normal, 0.0)).xyz );
  }


 
  // finalize color with provenance overlay
  vec3 in_out_color = get_color(in_position, normal, vec3(in_r, in_g, in_b), radius);

  // passed attributes: vertex shader -> geometry shader
  VertexOut.pass_ms_u = tangent;
  VertexOut.pass_ms_v = bitangent;
  VertexOut.pass_normal = normal;
  gl_Position = vec4(in_position, 1.0);
  VertexOut.pass_point_color = in_out_color;


  OPTIONAL_BEGIN
    vec4 pos_es = model_view_matrix * vec4(in_position, 1.0f);
    VertexOut.mv_vertex_position = pos_es.xyz;
  OPTIONAL_END
}
