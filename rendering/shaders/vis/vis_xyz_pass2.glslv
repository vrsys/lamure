// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 420 core


out VertexData {
    vec3 color;
    vec4 nor;
    float rad;
    float pointSize;
    float mv_vertex_depth;
} VertexOut;

uniform mat4 mvp_matrix;
uniform mat4 model_view_matrix;
uniform mat4 inv_mv_matrix;
uniform float height_divided_by_top_minus_bottom;
uniform float near_plane;
uniform float point_size_factor;


layout(location = 0) in vec3 in_position;
layout(location = 1) in float in_r;
layout(location = 2) in float in_g;
layout(location = 3) in float in_b;
layout(location = 4) in float empty;
layout(location = 5) in float in_radius;
layout(location = 6) in vec3 in_normal;


layout(location = 7) in float prov1;
layout(location = 8) in float prov2;
layout(location = 9) in float prov3;
layout(location = 10) in float prov4;
layout(location = 11) in float prov5;
layout(location = 12) in float prov6;

uniform bool show_normals;
uniform bool show_accuracy;
uniform float accuracy;

uniform int channel;
uniform bool heatmap;
uniform float heatmap_min;
uniform float heatmap_max;
uniform vec3 heatmap_min_color;
uniform vec3 heatmap_max_color;


vec3 quick_interp(vec3 color1, vec3 color2, float value) {
  return color1 + (color2 - color1) * clamp(value, 0, 1);
}


void main()
{


	if(in_radius == 0.0f)
	{
   		 gl_Position = vec4(2.0,2.0,2.0,1.0);
	}
	else
	{

      float scaled_radius = in_radius;

          vec4 normal = inv_mv_matrix * vec4(in_normal,0.0f);


	    {

		    vec4 pos_es = model_view_matrix * vec4(in_position, 1.0f);

		    float ps = 3.0f*(scaled_radius) * point_size_factor * (near_plane/-pos_es.z)* height_divided_by_top_minus_bottom;
         	gl_Position = mvp_matrix * vec4(in_position, 1.0);
           
            float prov_value = 0.0;

            if (show_normals) {
              vec4 vis_normal = normal;
              if( vis_normal.z < 0 ) {
                vis_normal = vis_normal * -1;
              }
              VertexOut.color = vec3(vis_normal.xyz * 0.5 + 0.5);
            }
            else if (channel == 0) {
              VertexOut.color = vec3(in_r, in_g, in_b);
            }
            else {
              if (channel == 1) {
                prov_value = prov1;
              }
              else if (channel == 2) {
                prov_value = prov2;
              }
              else if (channel == 3) {
                prov_value = prov3;
              }
              else if (channel == 4) {
                prov_value = prov4;
              }
              else if (channel == 5) {
                prov_value = prov5;
              }
              else if (channel == 6) {
                prov_value = prov6;
              }
              if (heatmap) {
                float value = (prov_value - heatmap_min) / (heatmap_max - heatmap_min);
			    VertexOut.color = quick_interp(heatmap_min_color, heatmap_max_color, value);
              }
              else {
                VertexOut.color = vec3(prov_value, prov_value, prov_value);
              }
			      }

            if (show_accuracy) {

              VertexOut.color = VertexOut.color + vec3(accuracy, 0.0, 0.0);
            }

        VertexOut.nor = normal;

		    gl_PointSize = ps;
		    VertexOut.pointSize = ps;


		    VertexOut.mv_vertex_depth = (pos_es).z;


		    VertexOut.rad = (scaled_radius) * point_size_factor;

	    }
	  }

}
