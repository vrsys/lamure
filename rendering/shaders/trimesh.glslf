// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 420 core

layout (location = 0) out vec4 out_color;

in vertex_data {
    vec4 position;
    vec4 normal;
    vec2 coord;
} vertex_in;

void main()
{
  out_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);

}

