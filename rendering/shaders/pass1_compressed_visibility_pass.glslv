// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 420 core

#extension GL_ARB_shader_storage_buffer_object : require



struct bvh_auxiliary {
  vec4 bb_and_rad_min;
  vec4 bb_and_rad_max;
};

layout(std430, binding = 1) buffer bvh_auxiliary_struct {
     bvh_auxiliary data_bvh[];
};

uniform mat4 inv_mv_matrix;
uniform float model_radius_scale;
uniform float point_size_factor;

layout(location = 0) in int in_qz_pos_x;
layout(location = 1) in int in_qz_pos_y;
layout(location = 2) in int in_qz_pos_z;
layout(location = 3) in int in_qz_normal_enumerator;
layout(location = 5) in int in_qz_radius;
/*
layout(location = 0) in vec3 in_position;
layout(location = 5) in float in_radius;
layout(location = 6) in vec3 in_normal;

*/


/*
uniform vec3 bb_min = vec3(0.0, 0.0, 0.0);
uniform vec3 bb_max = vec3(0.0, 0.0, 0.0);
uniform float rad_min = 0.0;
uniform float rad_max = 0.0;
*/
uniform int num_primitives_per_node;

out VertexData {
  vec3 pass_ms_u;
  vec3 pass_ms_v;
  vec3 pass_normal;
} VertexOut;



const ivec3 rgb_565_shift_vec   = ivec3(11, 5, 0);
const ivec3 rgb_565_mask_vec    = ivec3(0x1F, 0x3F, 0x1F);
const ivec3 rgb_565_quant_steps = ivec3(8, 4, 8);

const int num_points_u = 104;
const int num_points_v = 105;
const int total_num_points_per_face = 104 * 105;
const int face_divisor = 2;
void main()
{
 /* // dequantize color
  ivec3 rgb_multiplier = (ivec3(in_qz_color_rgb_565) >> rgb_565_shift_vec) & rgb_565_mask_vec;
  vec3 in_rgb = rgb_multiplier * rgb_565_quant_steps / 255.0;
*/

  int ssbo_node_id = gl_VertexID / num_primitives_per_node;

  bvh_auxiliary test = data_bvh[ssbo_node_id];

  // dequantize position
  vec3 in_position = test.bb_and_rad_min.xyz + ivec3(in_qz_pos_x, in_qz_pos_y, in_qz_pos_z) * ( (test.bb_and_rad_max.xyz - test.bb_and_rad_min.xyz)/65535.0 );

  // dequantize radius
  float in_radius = test.bb_and_rad_min.a + in_qz_radius * ( (test.bb_and_rad_max.a  - test.bb_and_rad_min.a) / 65535.0);

  // dummy normal dequantization
  //vec3 in_normal = vec3(0.5, 0.5, 0.5);

  // dequantized normal
  int compressed_normal_enumerator = in_qz_normal_enumerator;
  int face_id = compressed_normal_enumerator / (total_num_points_per_face);
  int is_main_axis_negative = ((face_id % 2) == 1 ) ? -1 : 1;
  compressed_normal_enumerator -= face_id * (total_num_points_per_face);
  int v_component = compressed_normal_enumerator / num_points_u;
  int u_component = compressed_normal_enumerator % num_points_u;

  int main_axis = face_id / face_divisor;

  float first_component = (u_component / float(num_points_u) ) * 2.0 - 1.0;
  float second_component = (v_component / float(num_points_v) ) * 2.0 - 1.0;

  vec3 unswizzled_components = vec3(first_component, 
                                    second_component,
                                    is_main_axis_negative * sqrt(-(first_component*first_component) - (second_component*second_component) + 1) );

  vec3 in_normal = unswizzled_components.zxy;

  if(1 == main_axis) {
    in_normal.xyz = unswizzled_components.yzx;
  } else if(2 == main_axis) {
    in_normal.xyz = unswizzled_components.xyz;    
  }
/*
  int first_comp_axis = (main_axis + 1) % 3;
  int second_comp_axis = (main_axis + 2) % 3;

  float first_component = (u_component / float(num_points_u) ) * 2.0 - 1.0;
  float second_component = (v_component / float(num_points_v) ) * 2.0 - 1.0;

  vec3 in_normal = vec3(first_component, 
                        second_component,
                        is_main_axis_negative * sqrt(-(first_component*first_component) - (second_component*second_component) + 1) );
*/
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

  VertexOut.pass_normal = normalize((inv_mv_matrix * vec4(in_normal, 0.0)).xyz);

  gl_Position = vec4(in_position, 1.0);

}