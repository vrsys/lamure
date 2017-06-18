// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 420 core

uniform mat4 model_to_screen_matrix;
uniform mat4 inv_mv_matrix;
uniform float model_radius_scale;
uniform float point_size_factor;
uniform int render_provenance;
uniform float average_radius;
uniform float accuracy;
uniform float radius_sphere;
uniform vec3 position_sphere;
uniform int state_lense;

layout(location = 0) in vec3 in_position;
layout(location = 1) in float in_r;
layout(location = 2) in float in_g;
layout(location = 3) in float in_b;
layout(location = 4) in float empty;
layout(location = 5) in float in_radius;
layout(location = 6) in vec3 in_normal;
layout(location = 7) in float prov_float;

const vec4 GREEN = vec4( 0.0, 1.0, 0.0, 1.0 );
const vec4 WHITE = vec4( 1.0, 1.0, 1.0, 1.0 );
const vec4 RED   = vec4( 1.0, 0.0, 0.0, 1.0 );

out VertexData {
  //output to geometry shader
  vec3 pass_ms_u;
  vec3 pass_ms_v;

  vec3 pass_point_color;
  vec3 pass_normal;
  float pass_prov_float;
} VertexOut;

bool is_in_sphere()
{
    return length(in_position - position_sphere) < radius_sphere;
    // return length((inv_mv_matrix * vec4(in_position, 1.0)).xyz - position_sphere) < radius_sphere;
}

float remap(float minval, float maxval, float curval)
{
    return (curval - minval ) / ( maxval - minval);
} 

void main()
{
  vec3 ms_n = normalize(in_normal.xyz);
  vec3 ms_u;

  //**compute tangent vectors**//
  if(ms_n.z != 0.0) {
    ms_u = vec3( 1, 1, (-ms_n.x -ms_n.y)/ms_n.z);
  } else if (ms_n.y != 0.0) {
    ms_u = vec3( 1, (-ms_n.x -ms_n.z)/ms_n.y, 1);
  } else {
    ms_u = vec3( (-ms_n.y -ms_n.z)/ms_n.x, 1, 1);
  }

  //**assign tangent vectors**//
  VertexOut.pass_ms_u = normalize(ms_u) * point_size_factor * model_radius_scale * in_radius;
  VertexOut.pass_ms_v = normalize(cross(ms_n, ms_u)) * point_size_factor * model_radius_scale * in_radius;

  VertexOut.pass_normal = normalize((inv_mv_matrix * vec4(in_normal, 0.0)).xyz );
  VertexOut.pass_prov_float = prov_float;

#if 1
  if(state_lense == 1 && is_in_sphere() && prov_float > 0.5f)
  // if(state_lense == 1 && is_in_sphere() && prov_float < 0.5f)
  {
    float u = clamp( prov_float, 0.0, 1.0 );
    if( u < 0.5 )
    {
        VertexOut.pass_point_color = mix( GREEN, WHITE, remap( 0.0, 0.5, u ) ).xyz;
    }
    else
    {
        VertexOut.pass_point_color = mix( WHITE, RED, remap( 0.5, 1.0, u ) ).xyz;
    }
    // VertexOut.pass_point_color = vec3(0.0, 1.0, 0.0);
  } else {
    VertexOut.pass_point_color = vec3(in_r, in_g, in_b);
  }
#else
  if(prov_float > 0.0)
  {
    VertexOut.pass_point_color = vec3(in_r, in_g, in_b);
  } else {
    VertexOut.pass_point_color = vec3(0.0, 1.0, 0.0);
  }
#endif

  gl_Position = vec4(in_position, 1.0);
}
