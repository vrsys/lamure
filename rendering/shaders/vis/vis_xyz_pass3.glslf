// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 420 core
 
layout(binding  = 0) uniform sampler2D in_color_texture;
layout(binding  = 1) uniform sampler2D in_normal_texture;
layout(binding  = 2) uniform sampler2D in_vs_position_texture;

layout(location = 0) out vec4 out_color;

uniform vec3 background_color;
     
in vec2 pos;

vec3 vs_light_pos = vec3(0.0, 0.0, 0.0);

vec3 LIGHT_DIFFUSE_COLOR = vec3(1.2, 1.2, 1.2);

float diffuse_power = 1.0;
float specular_power = 1.0;

vec3 shade(in vec3 vs_pos, in vec3 vs_normal, in vec3 vs_light_pos, in vec3 col) {

  vec3 light_dir = (vs_light_pos - vs_pos);
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

  float m = 1000.0;

  float specular_intensity = pow( max(NdotH, 0.0), m );

  return 
        vec3(0.1, 0.1, 0.1) + 
        diffuse_intensity * LIGHT_DIFFUSE_COLOR * col * 1.0

        +  specular_intensity * LIGHT_DIFFUSE_COLOR * col * 1.0;// / (0.0003 * light_distance);

  //float intensity = saturate(NdotL);
  //return 0.0005 * light_distance * vec3(1.0, 1.0, 1.0);
}

void main()	{

    out_color = vec4(background_color.r,background_color.g,background_color.b, 1.0f);

    vec4 texColor = texture2D(in_color_texture, (pos.xy + 1) / 2.0f);
	
	//optional
	vec3 texNormal 	   = texture2D(in_normal_texture, (pos.xy + 1) / 2.0f).xyz;
	vec3 texVSPosition = texture2D(in_vs_position_texture, (pos.xy + 1) / 2.0f).xyz;

    if(texColor.w != 0.0f) {
      texNormal = texNormal/texColor.w;
      texVSPosition.xyz = texVSPosition.xyz/texColor.w;
      texColor.xyz = (texColor.xyz/texColor.w);
      
      //out_color = vec4(texColor.xyz, 1.0);
      //out_color = vec4( 0.5 * (texNormal.xyz + 1.0) , 1.0);
      out_color = vec4( (texVSPosition.xyz), 1.0);



      vec3 shaded_color = shade(texVSPosition, texNormal, vs_light_pos, texColor.rgb);

      out_color = vec4(shaded_color,  1.0);
    }


 }
