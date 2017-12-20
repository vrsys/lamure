// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 420 core


in VertexData {
	vec3 color;
	vec4 nor;
  	float rad;
	float pointSize;
	float mv_vertex_depth;
  OPTIONAL_BEGIN
    vec3 mv_vertex_position;
  OPTIONAL_END
} VertexIn;


uniform bool ellipsify;
uniform bool clamped_normal_mode;
uniform float near_plane;
uniform float far_minus_near_plane;
uniform float max_deform_ratio;

layout(location = 0) out vec4 out_color;

OPTIONAL_BEGIN
  // shading
  INCLUDE ../common/shading/blinn_phong.glsl
OPTIONAL_END


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

	return -(xzRatio)*mappedPointCoord.x   - (yzRatio * mappedPointCoord.y);

}




float get_gaussianValue(float depth_offset, vec2 mappedPointCoord, vec3 newNormalVec)
{

    float radius;
    if(ellipsify)
    	radius =  mappedPointCoord.x*mappedPointCoord.x + mappedPointCoord.y*mappedPointCoord.y + depth_offset*depth_offset;
    else
    	radius =  mappedPointCoord.x*mappedPointCoord.x + mappedPointCoord.y*mappedPointCoord.y ;


    if(radius > 1.0)
	    discard;
    else
	    return 1.0f;
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


   float depth_offset = calc_depth_offset(mappedPointCoord, adjustedNormal) ;


   get_gaussianValue(depth_offset, mappedPointCoord, adjustedNormal);

    vec4 color_to_write = vec4(VertexIn.color.xyz, 1.0);

      //optional code for looking up shading attributes and performing shading
    OPTIONAL_BEGIN
      vec3 shaded_color = shade_blinn_phong(VertexIn.mv_vertex_position, adjustedNormal, 
                                            vec3(0.0, 0.0, 0.0), color_to_write.rgb);

      color_to_write = vec4(shaded_color,  1.0);
    OPTIONAL_END

   out_color = color_to_write;

}

