// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 420 core
 
layout(binding  = 0) uniform sampler2D in_color_texture;
layout(location = 0) out vec4 out_color;

uniform vec3 background_color;
     
in vec2 pos;

void main()	{

    out_color = vec4(background_color.r,background_color.g,background_color.b,1.0f);

    vec4 texColor = texture2D(in_color_texture, (pos.xy + 1) / 2.0f);
	

    if(texColor.w != 0.0f) {
      texColor = (texColor/texColor.w);
      out_color = vec4(pow(texColor.xyz, vec3(1.4f)), 1.0f);  
    }

 }
