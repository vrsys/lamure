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

// blinn-phong-shading in view space, => camera in 0,0,0
vec3 shade_blinn_phong(in vec3 vs_pos, in vec3 vs_normal, 
                       in vec3 vs_light_pos, in vec3 in_col) {

  vec4 ambient_light_color = vec4(0.5,0.5,0.5,1.0);


  vec3 light_dir = (vs_light_pos-vs_pos);
  float light_distance = length(light_dir);
  light_dir /= light_distance;

  light_distance = light_distance * light_distance;

  float NdotL = dot(vs_normal, light_dir);
  float diffuse_intensity =  max(0.0, NdotL);

  vec3 view_dir = (-vs_pos);

  // due to the normalize function's reciprocal square root
  vec3 H = normalize( light_dir + view_dir );

    //Intensity of the specular light
  float NdotH = dot( vs_normal, H );


  return diffuse_intensity * in_col * 1.0 + (in_col * ambient_light_color.rgb);
    
}

void main()
{
  vec4 n = vertex_in.normal;
  vec2 c = vertex_in.coord;

  vec3 nv = normalize((n*inverse(model_view_matrix)).xyz);
  vec3 color = vec3(c.r, c.g, 0.5);
  
  vec4 pos_es = model_view_matrix * vec4(vertex_in.position.xyz, 1.0f);
  color = shade_blinn_phong(pos_es.xyz, n.xyz, vec3(0.0, 0.0, 0.0), color);

  out_color = vec4(color, 1.0);
}

