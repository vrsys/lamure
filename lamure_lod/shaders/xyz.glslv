// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

layout (location = 0) in vec4 in_position;
layout (location = 1) in vec4 in_color;
layout (location = 2) in float in_radius;
layout (location = 3) in vec4 in_normal;

uniform mat4 inv_mv_matrix;
uniform float model_radius_scale;
uniform float point_size_factor;
uniform mat4 projection_matrix;
uniform mat4 mvp_matrix;
uniform mat4 model_view_matrix;


out VertexData {
  //output to fragment shader
  vec3 pass_point_color;
  vec3 pass_normal;
  vec2 pass_uv_coords;
} VertexOut;

void main()
{
  VertexOut.pass_normal = normalize((inv_mv_matrix * vec4(in_normal.xyz, 0.0)).xyz );

  gl_PointSize = 3.0;
  //gl_Position = vec4(0.5,0.5,0.5, 1.0);
  gl_Position = (projection_matrix * model_view_matrix) * vec4(in_position.xyz, 1.0);

  VertexOut.pass_point_color = in_color.rgb;
}

