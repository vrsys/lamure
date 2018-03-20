#version 440 core

out vec2 texture_coord;

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texture_coord;

//layout(binding = 0) uniform sampler2DArray physical_texture_array;
//layout(binding = 2) uniform usampler2D index_elevation;

//layout(std430, binding = 0) buffer out_feedback { int[] out_feedback_values; };

void main()
{
    texture_coord = in_texture_coord;

    /* VT PASS */

//    vec3 texture_coordinates = vec3(texture_coord, 0.0);
//    texture_coordinates.y = 1.0 - texture_coordinates.y;
//    uvec4 index_quadruple = texture(index_elevation, texture_coordinates.xy).rgba;
//    texture_coordinates.z = index_quadruple.w;
//    vec4 c;
//    uint current_level = index_quadruple.z;
//    uint tile_occupation_exponent = in_max_level_elevation - current_level;
//    uint occupied_index_pixel_per_dimension = uint(1 << tile_occupation_exponent);
//    uvec2 base_xy_offset = index_quadruple.xy;
//    vec2 dummy;
//    vec2 physical_tile_ratio_xy = modf((texture_coordinates.xy * in_index_dim_elevation / vec2(occupied_index_pixel_per_dimension)), dummy);
//    vec2 padding_scale = 1 - 2 * tile_padding / tile_size;
//    vec2 padding_offset = tile_padding / tile_size;
//    vec2 physical_texture_coordinates = (base_xy_offset.xy + physical_tile_ratio_xy * padding_scale + padding_offset) / physical_texture_dim;
//    c = texture(physical_texture_array, vec3(physical_texture_coordinates, texture_coordinates.z));
//    uint one_d_feedback_ssbo_index = base_xy_offset.x + base_xy_offset.y * physical_texture_dim.x + index_quadruple.w * physical_texture_dim.x * physical_texture_dim.y;
//    atomicAdd(out_feedback_values[one_d_feedback_ssbo_index], 10);

    gl_Position = projection_matrix * model_view_matrix * (vec4(in_position, 1.0));// + 0.05 * vec4(in_normal, 0.0) * (c.r + c.g + c.b) / 3.0);
}