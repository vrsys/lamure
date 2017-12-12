#version 330 core

in vec2 texture_coord;
flat in uint max_level;
flat in uint toggle_view;
flat in uvec2 physical_texture_dim;
flat in uvec2 index_texture_dim;

layout (std430, binding = 0) buffer out_feedback_ssbo{
  uint[] out_feedback_values;
};

layout(binding = 0) uniform sampler2D physical_texture;
layout(binding = 1) uniform usampler2D index_texture;

layout(location = 0) out vec4 out_color;

void main()
{
    // swap y axis
    vec2 swapped_y_texture_coordinates = texture_coord;
    swapped_y_texture_coordinates.y = 1.0 - swapped_y_texture_coordinates.y;

    vec4 c;

    uint reference_count = 0;
    if(toggle_view == 0)
    { // Show the physical texture
        c = texture(physical_texture, swapped_y_texture_coordinates);
        out_color = c;
    }
    else
    { // Show the image viewer

        // access on index texture, reading x,y,LoD into a uvec3 -> efficient
        uvec3 index_triplet = texture(index_texture, swapped_y_texture_coordinates).xyz;

        // extracting LoD from index texture
        uint current_level = index_triplet.z;

        // exponent for calculating the occupied pixels in our index texture, based on which level the tile is in
        uint tile_occupation_exponent = max_level - current_level;

        // 2^tile_occupation_exponent defines how many pixel (of the index texture) are used by the given tile
        uint occupied_index_pixel_per_dimension = uint(pow(2, tile_occupation_exponent));

        // offset represented as tiles is divided by total num tiles per axis
        // (replace max_width_tiles later by correct uniform)
        // extracting x,y from index texture
        uvec2 base_xy_offset = index_triplet.xy;

        // just to be conformant to the modf interface (integer parts are ignored)
        vec2 dummy;

        // base x,y coordinates * number of tiles / number of used index texture pixel
        // taking the factional part by modf

        vec2 physical_tile_ratio_xy = modf((swapped_y_texture_coordinates.xy * index_texture_dim / vec2(occupied_index_pixel_per_dimension)), dummy);

        // adding the ratio for every texel to our base offset to get the right pixel in our tile
        // and dividing it by the dimension of the phy. tex.

        vec2 physical_texture_coordinates = (base_xy_offset.xy + physical_tile_ratio_xy) / physical_texture_dim;

        // c = vec4(physical_tile_ratio_xy, 0.0, 1.0);

        // outputting the calculated coordinate from our physical texture
        c = texture(physical_texture, physical_texture_coordinates);

        // simple feedback
        uint one_d_feedback_ssbo_index = base_xy_offset.x + base_xy_offset.y * physical_texture_dim.x;

        reference_count = atomicAdd(out_feedback_values[one_d_feedback_ssbo_index], 1);
        reference_count += 1;

        // c = vec4( float(reference_count) / (10*375542.857 / float( (current_level+1) * (current_level+1) )), (float(reference_count) / (10*375542.857 / float(current_level+1) * (current_level+1) )) * 0.3, 0.0, 1.0 );
    }

    out_color = c;
}