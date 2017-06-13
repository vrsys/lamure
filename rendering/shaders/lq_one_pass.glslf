// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 420 core

uniform int state_lense;
uniform int width_window;
uniform int height_window;
uniform float radius_sphere_screen;
uniform vec2 position_sphere_screen;

in VertexData {
    //output to fragment shader
    vec3 pass_point_color;
    vec3 pass_normal;
    vec2 pass_uv_coords;
    float pass_prov_float;
} VertexIn;

layout(location = 0) out vec4 out_color;

bool is_in_sphere()
{
    float height_width = float(height_window) / float(width_window);
    float width_height = float(width_window) / float(height_window);

    float x = gl_FragCoord.x/float(width_window);
    float y = gl_FragCoord.y/float(height_window);
    float z = gl_FragCoord.z; // Already in range [0,1]

    // Converting from range [0,1] to NDC [-1,1]
    float ndcx = x * 2.0 - 1.0;
    float ndcy = y * 2.0 - 1.0;
    float ndcz = z * 2.0 - 1.0;
    vec2 ndc = vec2(ndcx, ndcy);
    ndc.x = ndc.x * width_height;

    return length(ndc - position_sphere_screen) < radius_sphere_screen;
}

void main() {
    vec2 uv_coords = VertexIn.pass_uv_coords;

    if ( dot(uv_coords, uv_coords)> 1 )
    {
        discard;
    }

    if(state_lense == 2 && is_in_sphere() && VertexIn.pass_prov_float < 0.5f)
    // if(state_lense == 2 && is_in_sphere() && VertexIn.pass_prov_float > 0.5f)
    {
        out_color = vec4(0.0, 1.0, 0.0, 1.0);
    } else {
        out_color = vec4(pow(VertexIn.pass_point_color, vec3(1.4,1.4,1.4)), 1.0);
    }
}


