// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 450 core

uniform mat4 mvp_matrix;
uniform mat4 model_matrix;
uniform mat4 model_view_matrix;
uniform mat4 inv_mv_matrix;
uniform mat4 model_to_screen_matrix;

uniform float near_plane;

uniform bool face_eye;
uniform vec3 eye;
uniform float max_radius;

uniform float point_size_factor;
uniform float model_radius_scale;

uniform int fem_result;
uniform int fem_vis_mode;
uniform float fem_deform_factor;
uniform float fem_min_absolute_deform;
uniform float fem_max_absolute_deform;
uniform vec3 fem_min_deformation;
uniform vec3 fem_max_deformation;


uniform float time_cursor_pos = 0.0; //float between 0 and num_timesteps for a simulation
uniform int num_vertices_in_fem = 0;
uniform int num_attributes_in_fem = 10; //e.g. 3x pos + 1x Sig_XX  would be 4, required to compute the stride to next timestep

uniform float current_min_color_attrib = 0.0;
uniform float current_max_color_attrib = 0.0;

uniform int current_attribute_id = 0;

uniform int max_timestep_id = 0; // for static simulation, this is 0, for dynamic simulations this should be NUM_TIMESTEPS-1

uniform int color_attribute_index = 0;  //attribute which should be sampled for heatmap coloration

layout(std430, binding = 10)  coherent readonly buffer fem_data_array_struct {
     float fem_array[];
};


layout(location = 0) in vec3 in_position;
layout(location = 1) in float in_r;
layout(location = 2) in float in_g;
layout(location = 3) in float in_b;
layout(location = 4) in float empty;
layout(location = 5) in float in_radius;
layout(location = 6) in vec3 in_normal;

layout(location = 7) in int fem_vert_id_0;
layout(location = 8) in int fem_vert_id_1;
layout(location = 9) in int fem_vert_id_2;
layout(location = 10) in float fem_vert_w_0;
layout(location = 11) in float fem_vert_w_1;
layout(location = 12) in float fem_vert_w_2;



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


INCLUDE fem_vis_ssbo_color.glsl

vec3 unpack_fem_result(float v){
  vec3 res = vec3(fract(v), fract(v * 256.0), fract(v * 65536.0));
  res *= (fem_max_deformation - fem_min_deformation);
  res += fem_min_deformation;
  res *= 2.0;
  res -= 1.0;
  return res;
}

void main() {
  float radius = in_radius;
  if (radius > max_radius) {
    radius = max_radius;
  }

  vec3 new_in_position = in_position;
  vec3 deformation = vec3(0.0);
  float deform = 0.0;
  if(0 != fem_result){

/*
    if(1 == fem_result){
      deform = prov1;
    }
    else if(2 == fem_result){
      deform = prov2;
    }
    else if(3 == fem_result){
      deform = prov3;
    }
    else if(4 == fem_result){
      deform = prov4;
    }
    else if(5 == fem_result){
      deform = prov5;
    }
    else if(6 == fem_result){
      deform = prov6;
    }
    else if(7 == fem_result){
      deform = prov7;
    }
    else if(8 == fem_result){
      deform = prov8;
    }
    else if(9 == fem_result){
      deform = prov9;
    }

*/

    if(0.0 != deform){
      deformation = unpack_fem_result(deform);

      if(1 == fem_vis_mode){
        new_in_position += fem_deform_factor * deformation;
      }

    }
  }


  vec3 normal = in_normal;
  if (face_eye) {
    normal = normalize(eye-(model_matrix*vec4(new_in_position, 1.0)).xyz);
  }
 
  // precalculate tangent vectors to establish the surfel shape
  vec3 tangent   = vec3(0.0);
  vec3 bitangent = vec3(0.0);
  compute_tangent_vectors(normal, radius, tangent, bitangent);

  vec3 ms_n = normalize(normal.xyz);
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
  VertexOut.pass_ms_u = normalize(ms_u) * point_size_factor * model_radius_scale * radius;
  VertexOut.pass_ms_v = normalize(cross(ms_n, ms_u)) * point_size_factor * model_radius_scale * radius;

  if (!face_eye) {
    normal = normalize((inv_mv_matrix * vec4(normal, 0.0)).xyz );
  }

  VertexOut.pass_normal = normal;

  VertexOut.pass_point_color = get_color(new_in_position, normal, vec3(in_r, in_g, in_b), radius);

  if(0 != fem_result && 0 == fem_vis_mode && 0.0 != deform){
    VertexOut.pass_point_color = mix(VertexOut.pass_point_color, data_value_to_rainbow(length(deformation), fem_min_absolute_deform, fem_max_absolute_deform), 0.3);
  }

  if(0 != fem_result && 1 == fem_vis_mode && 0.0 != deform){
    VertexOut.pass_point_color = mix(VertexOut.pass_point_color, data_value_to_rainbow(length(deformation), fem_min_absolute_deform, fem_max_absolute_deform), 0.3);
  }
  

  //number of float attributes (because FEM ssbo is a float array)


  if(fem_result > 0) {
    //only do FEM vis, if there are valid weights
    if( !(fem_vert_w_0 <= 0.0f && fem_vert_w_1 <= 0.0f && fem_vert_w_2 <= 0.0f) ) {
      //init_colormap();


      uint num_elements_per_timestep = num_attributes_in_fem * num_vertices_in_fem;


      ;



      uint timestep_x = uint(mod(time_cursor_pos, float(max_timestep_id) ) );// 2;
      uint timestep_x_plus_1 = min(uint(timestep_x) + 1, max_timestep_id);



      // attributes other than the deformation along axis need to have an index offset of 2, because the x,y and z deforms are the only ones that are interleaved
      

      //color_attribute_index
      uint attribute_base_offset_t_x = num_vertices_in_fem; //TBC

      float spatially_mixed_attrib_t_x = fem_array[num_elements_per_timestep * timestep_x + num_vertices_in_fem * (current_attribute_id) + fem_vert_id_0] * fem_vert_w_0 
                                       + fem_array[num_elements_per_timestep * timestep_x + num_vertices_in_fem * (current_attribute_id) + fem_vert_id_1] * fem_vert_w_1 
                                       + fem_array[num_elements_per_timestep * timestep_x + num_vertices_in_fem * (current_attribute_id) + fem_vert_id_2] * fem_vert_w_2; 

      float spatially_mixed_attrib_t_x_plus_1 = fem_array[num_elements_per_timestep * timestep_x_plus_1 + num_vertices_in_fem * (current_attribute_id) + fem_vert_id_0] * fem_vert_w_0 
                                              + fem_array[num_elements_per_timestep * timestep_x_plus_1 + num_vertices_in_fem * (current_attribute_id) + fem_vert_id_1] * fem_vert_w_1 
                                              + fem_array[num_elements_per_timestep * timestep_x_plus_1 + num_vertices_in_fem * (current_attribute_id) + fem_vert_id_2] * fem_vert_w_2; 

      float temporal_weight = time_cursor_pos - timestep_x;

      float spatio_temporally_mixed_attribute = mix(spatially_mixed_attrib_t_x, spatially_mixed_attrib_t_x_plus_1, temporal_weight);

      //float normalized_attrib = (spatially_mixed_attrib_t_x - current_min_color_attrib) /  ( current_max_color_attrib - current_min_color_attrib );
      

      VertexOut.pass_point_color = mix(VertexOut.pass_point_color, data_value_to_rainbow(spatio_temporally_mixed_attribute, current_min_color_attrib, current_max_color_attrib), 0.3);
      //VertexOut.pass_point_color = get_colormap_value(normalized_attrib);
 
    }
  }

  gl_Position = vec4(new_in_position, 1.0);


  OPTIONAL_BEGIN
    vec4 pos_es = model_view_matrix * vec4(new_in_position, 1.0f);
    VertexOut.mv_vertex_position = pos_es.xyz;
  OPTIONAL_END
}