// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 420 core
 
layout(binding  = 0) uniform sampler2D in_color_texture;
layout(binding  = 1) uniform sampler2D in_normal_texture;


layout(location = 0) out vec4 out_color;
layout(location = 1) out vec3 out_normal;
        

uniform int renderMode;

//for texture access
in vec2 pos;


void main()	
{

	if(renderMode == 0 || renderMode == 1) //color mode
	{
		vec4 texColor = texture2D(in_color_texture, (pos.xy + 1) / 2.0f);

		if(texColor.w != 0.0f) {
			texColor.xyz = (texColor.xyz/texColor.w);

			vec4 texNormal = texture2D(in_normal_texture, (pos.xy + 1) / 2.0f);
			texNormal.xyz = (texNormal.xyz/texColor.w);

			out_color  = vec4(pow(texColor.xyz, vec3(1.4f)), 1.0f);
			out_normal = vec3(texNormal.xyz);
		} else {
			
			out_color = vec4(0.f,0.0f,0.0f,1.0f);
			out_normal = vec3(0.0, 0.0, 0.0);
		}
	 
		

	}
	else if(renderMode == 1) //debug ooc mode
	{

	    vec4 texColor = texture2D(in_color_texture, (pos.xy + 1) / 2.0f);
		texColor = texColor/100000000;//(texColor/texColor.w);

	    if(texColor.w != 0.0f) { 

	    	out_color = vec4(texColor.xyz, 1.0f);  
	    
	    } else {
			out_color = vec4(0.2f,0.2f,0.2f,1.0f);
	    }
	 
		

	}




 }
