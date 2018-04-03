#version 440 core

out vec2 texture_coord;

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texture_coord;

uniform uint max_level;
uniform uvec2 physical_texture_dim;

uniform vec2 tile_size;
uniform vec2 tile_padding;

layout(binding = 17) uniform sampler2DArray physical_texture_array;
layout(binding = 18) uniform usampler2D elevation_idx_texture;

layout(std430, binding = 0) buffer out_lod_feedback { int out_lod_feedback_values[]; };

void main()
{
    texture_coord = in_texture_coord;

    /* VT PASS */

    vec2 texture_coordinates = vec2(texture_coord);
    texture_coordinates.y = 1.0 - texture_coordinates.y;

    uvec4 index_quadruple = texture(elevation_idx_texture, texture_coordinates).rgba;

    // exponent for calculating the occupied pixels in our index texture, based on which level the tile is in
    uint tile_occupation_exponent = max_level - 3;

    // 2^tile_occupation_exponent defines how many pixel (of the index texture) are used by the given tile
    uint occupied_index_pixel_per_dimension = uint(1 << tile_occupation_exponent);

    // offset represented as tiles is divided by total num tiles per axis
    // (replace max_width_tiles later by correct uniform)
    // extracting x,y from index texture
    uvec2 base_xy_offset = index_quadruple.xy;

    // base x,y coordinates * number of tiles / number of used index texture pixel
    // taking the factional part by modf
    vec2 physical_tile_ratio_xy = fract(texture_coordinates * (1 << max_level) / vec2(occupied_index_pixel_per_dimension));

    // Use only tile_size - 2*tile_padding pixels to render scene
    // Therefore, scale reduced tile size to full size and translate it
    vec2 padding_scale = 1 - 2 * tile_padding / tile_size;
    vec2 padding_offset = tile_padding / tile_size;

    // adding the ratio for every texel to our base offset to get the right pixel in our tile
    // and dividing it by the dimension of the phy. tex.
    vec2 physical_texture_coordinates = (base_xy_offset.xy + physical_tile_ratio_xy * padding_scale + padding_offset) / physical_texture_dim;

    // outputting the calculated coordinate from our physical texture
    vec4 c = texture(physical_texture_array, vec3(physical_texture_coordinates, index_quadruple.z));

    gl_Position = projection_matrix * model_view_matrix * (vec4(in_position, 1.0) + 0.05 * vec4(in_normal, 0.0) * (c.r + c.g + c.b) / 3.0);

    uint one_d_feedback_ssbo_index = index_quadruple.x + index_quadruple.y * physical_texture_dim.x + index_quadruple.z * physical_texture_dim.x * physical_texture_dim.y;
    atomicMax(out_lod_feedback_values[one_d_feedback_ssbo_index], 3);
}