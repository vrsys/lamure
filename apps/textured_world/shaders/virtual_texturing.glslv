#version 440 core

out vec2 texture_coord;
flat out uvec2 physical_texture_dim;
flat out uvec2 index_texture_dim;
flat out uint max_level;
flat out uint toggle_view;

flat out vec2 tile_size;
flat out vec2 tile_padding;

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;
uniform uint in_max_level;
uniform int in_toggle_view;
uniform uvec2 in_physical_texture_dim;
uniform uvec2 in_index_texture_dim;

uniform uint in_tile_size;
uniform uint in_tile_padding;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texture_coord;

layout(binding = 0) uniform sampler2DArray physical_texture_array;
layout(binding = 1) uniform usampler2D index_texture;

void main()
{
    tile_size = vec2(in_tile_size);
    tile_padding = vec2(in_tile_padding);

    max_level = in_max_level;
    toggle_view = in_toggle_view;
    texture_coord = in_texture_coord;
    index_texture_dim = in_index_texture_dim;
    physical_texture_dim = in_physical_texture_dim;

    /* VT PASS */

    vec3 texture_coordinates = vec3(texture_coord, 0.0);
    texture_coordinates.y = 1.0 - texture_coordinates.y;
    uvec4 index_quadruple = texture(index_texture, texture_coordinates.xy).rgba;
    texture_coordinates.z = index_quadruple.w;
    vec4 c;
    uint current_level = index_quadruple.z;
    uint tile_occupation_exponent = max_level - current_level;
    uint occupied_index_pixel_per_dimension = uint(1 << tile_occupation_exponent);
    uvec2 base_xy_offset = index_quadruple.xy;
    vec2 dummy;
    vec2 physical_tile_ratio_xy = modf((texture_coordinates.xy * index_texture_dim / vec2(occupied_index_pixel_per_dimension)), dummy);
    vec2 padding_scale = 1 - 2 * tile_padding / tile_size;
    vec2 padding_offset = tile_padding / tile_size;
    vec2 physical_texture_coordinates = (base_xy_offset.xy + physical_tile_ratio_xy * padding_scale + padding_offset) / physical_texture_dim;
    c = texture(physical_texture_array, vec3(physical_texture_coordinates, texture_coordinates.z));

    gl_Position = projection_matrix * model_view_matrix * (vec4(in_position, 1.0) + 0.05 * vec4(in_normal, 0.0) * (c.r + c.g + c.b) / 3.0);
}