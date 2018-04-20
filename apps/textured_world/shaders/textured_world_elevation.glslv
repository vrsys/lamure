#version 440 core

out vec2 texture_coord;

uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texture_coord;

uniform uint elevation_max_level;
uniform uvec2 physical_texture_dim;

uniform vec2 tile_size;
uniform vec2 tile_padding;

uniform int enable_displacement;

layout(binding = 17) uniform sampler2DArray physical_texture_array;

layout(std430, binding = 0) buffer out_lod_feedback { int out_lod_feedback_values[]; };

layout(binding = 18) uniform usampler2D hierarchical_idx_textures_elevation[16];

struct idx_tex_positions
{
    int parent_lvl;
    uvec4 parent_idx;
    int child_lvl;
    uvec4 child_idx;
};

vec4 get_physical_texture_color(uvec4 index_quadruple, vec2 texture_sampling_coordinates, uint current_level)
{
    // exponent for calculating the occupied pixels in our index texture, based on which level the tile is in
    uint tile_occupation_exponent = elevation_max_level - current_level;

    // 2^tile_occupation_exponent defines how many pixel (of the index texture) are used by the given tile
    uint occupied_index_pixel_per_dimension = uint(1 << tile_occupation_exponent);

    // offset represented as tiles is divided by total num tiles per axis
    // (replace max_width_tiles later by correct uniform)
    // extracting x,y from index texture
    uvec2 base_xy_offset = index_quadruple.xy;

    // base x,y coordinates * number of tiles / number of used index texture pixel
    // taking the factional part by modf
    vec2 physical_tile_ratio_xy = fract(texture_sampling_coordinates * (1 << elevation_max_level) / vec2(occupied_index_pixel_per_dimension));

    // Use only tile_size - 2*tile_padding pixels to render scene
    // Therefore, scale reduced tile size to full size and translate it
    vec2 padding_scale = 1 - 2 * tile_padding / tile_size;
    vec2 padding_offset = tile_padding / tile_size;

    // adding the ratio for every texel to our base offset to get the right pixel in our tile
    // and dividing it by the dimension of the phy. tex.
    vec2 physical_texture_coordinates = (base_xy_offset.xy + physical_tile_ratio_xy * padding_scale + padding_offset) / physical_texture_dim;

    // outputting the calculated coordinate from our physical texture
    vec4 c = texture(physical_texture_array, vec3(physical_texture_coordinates, index_quadruple.z));

    return c;
}

void update_feedback(int feedback_value, uvec4 base_offset)
{
    uint one_d_feedback_ssbo_index = base_offset.x + base_offset.y * physical_texture_dim.x + base_offset.z * physical_texture_dim.x * physical_texture_dim.y;

    atomicMax(out_lod_feedback_values[one_d_feedback_ssbo_index], feedback_value);
}

vec4 mix_colors(idx_tex_positions positions, int desired_level, vec2 texture_coordinates, float mix_ratio)
{
    vec4 child_color = get_physical_texture_color(positions.child_idx, texture_coordinates, positions.child_lvl);
    vec4 parent_color = get_physical_texture_color(positions.parent_idx, texture_coordinates, positions.parent_lvl);

    return mix(parent_color, child_color, mix_ratio);
}

vec4 traverse_idx_hierarchy(float lambda, vec2 texture_coordinates)
{
    float mix_ratio = fract(lambda);
    int desired_level = int(ceil(lambda));

    idx_tex_positions positions;

    // Desired level can be negative when the dxdy-fct requests a coarser representation as of the root tile size
    if(desired_level <= 0)
    {
        uvec4 idx_pos = texture(hierarchical_idx_textures_elevation[0], texture_coordinates).rgba;
        positions = idx_tex_positions(0, idx_pos, 0, idx_pos);
    }
    else
    {
        // Go from desired tree level downwards to root until a loaded tile is found
        for(int i = desired_level; i >= 0; --i)
        {
            uvec4 idx_child_pos = texture(hierarchical_idx_textures_elevation[i], texture_coordinates).rgba;

            // check if the requested tile is loaded and if we are not at the root level
            // enables to mix (linearly interpolate) between hierarchy levels
            if(idx_child_pos.w == 1 && i >= 1)
            {
                uvec4 idx_parent_pos = texture(hierarchical_idx_textures_elevation[i - 1], texture_coordinates).rgba;
                positions = idx_tex_positions(i - 1, idx_parent_pos, i, idx_child_pos);
                break;
            }

            // we are down to the root level: we cannot take the parent node from the root node;
            // therefore, we use the root node as child as well as parent for mixing
            else if(idx_child_pos.w == 1 && i == 0)
            {
                positions = idx_tex_positions(0, idx_child_pos, 0, idx_child_pos);
                break;
            }
        }
    }

    vec4 c;
    c = mix_colors(positions, desired_level, texture_coordinates, mix_ratio);

    int feedback_value = desired_level;
    update_feedback(feedback_value, positions.child_idx);

    return c;
}

void main()
{
    texture_coord = in_texture_coord;

    vec2 texture_coordinates = vec2(texture_coord);
    texture_coordinates.y = 1.0 - texture_coordinates.y;

    vec4 elevation;

    float lambda = 4;
    elevation = traverse_idx_hierarchy(lambda, texture_coordinates);

    vec4 displacement = vec4(0.0);

    float normalized_elevation = (elevation.r - 0.5) * 2.0;

    if (1 == enable_displacement) {
      displacement = 0.0125 * vec4(in_normal, 0.0) * normalized_elevation;
    }

    gl_Position = projection_matrix * model_view_matrix * (vec4(in_position, 1.0) + displacement);
}