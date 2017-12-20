// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 420 core

  //optional 

in VertexData {
	vec3 color;
	vec4 nor;
  	float rad;
	float pointSize;
	float mv_vertex_depth;
  vec3 mv_vertex_position;
} VertexIn;

layout(binding  = 0) uniform sampler2D depth_texture_pass1;
layout(binding  = 1) uniform sampler2D pointsprite_texture;

uniform vec2 win_size;
uniform float max_deform_ratio;

uniform bool ellipsify;
uniform bool clamped_normal_mode;

uniform float near_plane;
uniform float far_minus_near_plane;

layout(location = 0) out vec4 accumulated_colors;

//optional
layout(location = 1) out vec3 accumulated_normals;
layout(location = 2) out vec3 accumulated_vs_positions;

#define NORMAL_Z_OFFSET 0.00000001f


float calc_depth_offset(vec2 mappedPointCoord, vec3 adjustedNormal)
{
  float xzRatio = (adjustedNormal.x/adjustedNormal.z);
  float yzRatio = (adjustedNormal.y/adjustedNormal.z);

if(clamped_normal_mode)
{
	float zBound = max_deform_ratio;
	float normalZ = adjustedNormal.z;

	if(normalZ > 0.0)
		normalZ = max(zBound, normalZ);
	else
		normalZ = -max(zBound, -normalZ);

	xzRatio = (adjustedNormal.x/normalZ);
	yzRatio = (adjustedNormal.y/normalZ);
}


	return -(xzRatio)* mappedPointCoord.x   - (yzRatio * mappedPointCoord.y);
}


float get_gaussianValue(float depth_offset, vec2 mappedPointCoord, vec3 newNormalVec)
{
    float radius;
    if(ellipsify)
    	radius = sqrt(mappedPointCoord.x*mappedPointCoord.x + mappedPointCoord.y*mappedPointCoord.y + depth_offset*depth_offset);
    else
    	radius = sqrt(mappedPointCoord.x*mappedPointCoord.x + mappedPointCoord.y*mappedPointCoord.y) ;

    vec3 gauss = texture(pointsprite_texture, vec2(radius,1.0f)).rgb;


    if(radius >= 1.0f )
	discard;
    else
	return gauss.r;

}


void main()
{

   vec3 adjustedNormal = vec3(0.0,0.0,0.0);
   if(VertexIn.nor.z < 0)
   {
	//discard;
	   adjustedNormal = VertexIn.nor.xyz * -1;
   }
   else
   {
    adjustedNormal = VertexIn.nor.xyz;
   }

   vec2 mappedPointCoord = gl_PointCoord*2 + vec2(-1.0f, -1.0f);

   float depth_offset = calc_depth_offset(mappedPointCoord, adjustedNormal);

   float weight = get_gaussianValue(depth_offset, mappedPointCoord, adjustedNormal) / (VertexIn.rad*VertexIn.rad) ;


   float depthValue =  texture2D(depth_texture_pass1, gl_FragCoord.xy/win_size.xy).r;


   float depth_to_compare;


if(ellipsify)
   depth_to_compare = VertexIn.mv_vertex_depth + depth_offset*VertexIn.rad;
else
  depth_to_compare = VertexIn.mv_vertex_depth;
  depthValue = (-depthValue * 1.0 * far_minus_near_plane) + near_plane;

   if( depthValue  - (depth_to_compare)    < 0.00031  + 3.0*(VertexIn.rad) )
   {

	   accumulated_colors  = vec4(VertexIn.color * weight, weight);

     //optional
     accumulated_normals = vec3(adjustedNormal.xyz * weight);
     accumulated_vs_positions = vec3(VertexIn.mv_vertex_position.xyz * weight);
      //accumulated_colors = vec4((adjustedNormal + vec3(1.0))*0.5*weight, weight);
   }
   else
  	discard;



}

