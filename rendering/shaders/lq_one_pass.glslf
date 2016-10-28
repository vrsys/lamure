// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 420 core


in VertexData {
  //output to fragment shader
  vec3 pass_point_color;
  vec3 pass_normal;
  vec2 pass_uv_coords;
} VertexIn;

layout(location = 0) out vec4 out_color;

void main() {
  vec2 uv_coords = VertexIn.pass_uv_coords;

  if ( dot(uv_coords, uv_coords)> 1 ) {
    discard;
  }

#if 0
  vec3 normal = VertexIn.pass_normal;
  if ( normal.z < 0 ) {
    normal *= -1.0; 
  }

  normal = (normal + vec3(1.0, 1.0, 1.0)) / 2.0;
#endif

  out_color = vec4(pow(VertexIn.pass_point_color, vec3(1.4,1.4,1.4)), 1.0);
}


