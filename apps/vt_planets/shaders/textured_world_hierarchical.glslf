// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#version 440 core

// #define RASTERIZATION_COUNT
// #define HIGHLIGHT_FEEDBACK_POINT

// #define FINE_DERIVATIVES
#extension GL_ARB_derivative_control : enable

const int ROW_SKIP = 64;
const int COLUMN_SKIP = 64;

in vec2 texture_coord;

uniform uint max_level;
uniform uvec2 physical_texture_dim;

uniform vec2 tile_size;
uniform vec2 tile_padding;

uniform bool enable_hierarchy;
uniform int toggle_visualization;

layout(binding = 0) uniform usampler2D hierarchical_idx_textures[16];
layout(binding = 17) uniform sampler2DArray physical_texture_array;

uniform bool enable_feedback;
layout(std430, binding = 0) buffer out_lod_feedback { int out_lod_feedback_values[]; };
layout(std430, binding = 2) buffer in_feedback_inv_index { uint in_feedback_inv_index_values[]; };

#ifdef RASTERIZATION_COUNT
layout(std430, binding = 1) buffer out_count_feedback { uint out_count_feedback_values[]; };
#endif

layout(location = 0) out vec4 out_color;

struct idx_tex_positions
{
    uint parent_lvl;
    uvec4 parent_idx;
    uint child_lvl;
    uvec4 child_idx;
};

/*
 * Estimation of the requiered Level-of-Detail.
 */
float dxdy(vec2 texture_sampling_coordinates)
{
    vec2 c = texture_sampling_coordinates;

#ifdef FINE_DERIVATIVES
    float dFdxCx = dFdxFine(c.x);
    float dFdxCy = dFdxFine(c.y);

    float dFdyCx = dFdyFine(c.x);
    float dFdyCy = dFdyFine(c.y);
#else
    float dFdxCx = dFdxCoarse(c.x);
    float dFdxCy = dFdxCoarse(c.y);

    float dFdyCx = dFdyCoarse(c.x);
    float dFdyCy = dFdyCoarse(c.y);
#endif

    float dx_sq_norm = dFdxCx * dFdxCx + dFdxCy * dFdxCy;
    float dy_sq_norm = dFdyCx * dFdyCx + dFdyCy * dFdyCy;

    float sqrho = max(1.f / dx_sq_norm, 1.f / dy_sq_norm);

    if(isinf(sqrho) || isnan(sqrho))
    {
        return -float(max_level);
    }

    return 0.5f * log2(tile_size.x * tile_size.x / sqrho); //a valid trick to avoid sqrt and two log2s
}

/*
 * Physical texture lookup
 */
vec4 get_physical_texture_color(uvec4 index_quadruple, vec2 texture_sampling_coordinates, uint current_level)
{
    // base x,y coordinates * number of tiles / number of used index texture pixel
    // taking the factional part by modf
    vec2 physical_tile_ratio_xy = fract(texture_sampling_coordinates * (1 << current_level));

    // Use only tile_size - 2*tile_padding pixels to render scene
    // Therefore, scale reduced tile size to full size and translate it
    vec2 padding_scale = 1 - 2 * tile_padding / tile_size;
    vec2 padding_offset = tile_padding / tile_size;

    // adding the ratio for every texel to our base offset to get the right pixel in our tile
    // and dividing it by the dimension of the phy. tex.
    vec2 physical_texture_coordinates = (index_quadruple.xy + physical_tile_ratio_xy * padding_scale + padding_offset) / physical_texture_dim;

    // outputting the calculated coordinate from our physical texture
    return texture(physical_texture_array, vec3(physical_texture_coordinates, index_quadruple.z));
}

/*
 * Fill the feedback buffer with the feedback value for a given tile.
 * Here, we use the maximum lod required from the rendered tile.
 */
void update_feedback(int feedback_value, uvec4 base_offset)
{
    uint one_d_feedback_ssbo_index = base_offset.x + base_offset.y * physical_texture_dim.x + base_offset.z * physical_texture_dim.x * physical_texture_dim.y;
    uint compact_position = in_feedback_inv_index_values[one_d_feedback_ssbo_index];

    uint prev = out_lod_feedback_values[compact_position];
    if(prev < feedback_value)
    {
        out_lod_feedback_values[compact_position] = feedback_value;
    }
}

/*
 * Interpolate linearly between the parent and child physical texels.
 */
vec4 mix_colors(idx_tex_positions positions, vec2 texture_coordinates, float mix_ratio)
{
    vec4 child_color = get_physical_texture_color(positions.child_idx, texture_coordinates, positions.child_lvl);
    vec4 parent_color = get_physical_texture_color(positions.parent_idx, texture_coordinates, positions.parent_lvl);

    return enable_hierarchy == true ? mix(parent_color, child_color, mix_ratio) : child_color;
}

vec4 illustrate_level(int desired_level, float mix_ratio, idx_tex_positions positions)
{
    vec4 child_color = vec4(0, 0, 0, 1);
    vec4 parent_color = vec4(0, 0, 0, 1);

    if(toggle_visualization == 1)
    {
        vec4 c0 = vec4(0, 0, 0, 1); // black   - level 0 and below
        vec4 c1 = vec4(0, 0, 1, 1); // blue    - level 1, 8, 15
        vec4 c2 = vec4(0, 1, 0, 1); // green   - level 2, 9, 16
        vec4 c3 = vec4(0, 1, 1, 1); // cyan    - level 3, 10
        vec4 c4 = vec4(1, 0, 0, 1); // red     - level 4, 11
        vec4 c5 = vec4(1, 0, 1, 1); // magenta - level 5, 12
        vec4 c6 = vec4(1, 1, 0, 1); // yellow  - level 6, 13
        vec4 c7 = vec4(1, 1, 1, 1); // white   - level 7, 14

        switch(desired_level)
        {
        case -2:
        case -1:
        case 0:
            parent_color = c0;
            child_color = c0;
            break;
        case 1:
            parent_color = c0;
            child_color = c1;
            break;
        case 2:
        case 9:
        case 16:
            parent_color = c1;
            child_color = c2;
            break;
        case 3:
        case 10:
            parent_color = c2;
            child_color = c3;
            break;
        case 4:
        case 11:
            parent_color = c3;
            child_color = c4;
            break;
        case 5:
        case 12:
            parent_color = c4;
            child_color = c5;
            break;
        case 6:
        case 13:
            parent_color = c5;
            child_color = c6;
            break;
        case 7:
        case 14:
            parent_color = c6;
            child_color = c7;
            break;
        case 8:
        case 15:
            parent_color = c7;
            child_color = c1;
        }

        // return vec4(lambda / 16.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        vec4 child_idx = positions.child_idx;
        float child_lvl = positions.child_lvl;
        vec4 parent_idx = positions.parent_idx;
        float parent_lvl = positions.parent_lvl;

        float array_size = textureSize(physical_texture_array, 0).z;

        child_color = vec4(clamp((child_lvl / 8.0), 0.0, 1.0),
                           (float(child_idx.x + child_idx.y + child_idx.z) / float(physical_texture_dim.x + physical_texture_dim.y + array_size)),
                           clamp(((child_lvl - 8.0) / 8.0), 0.0, 1.0),
                           1);

        parent_color = vec4(clamp((parent_lvl / 8.0), 0.0, 1.0),
                            (float(parent_idx.x + parent_idx.y + parent_idx.z) / float(physical_texture_dim.x + physical_texture_dim.y + array_size)),
                            clamp(((parent_lvl - 8.0) / 8.0), 0.0, 1.0),
                            1);
    }

    return enable_hierarchy == true ? mix(parent_color, child_color, mix_ratio) : child_color;
}

/*
 * Traverse the index hierarchy textures to find the best representation for the given texture coordinates
 * based on the optimal representation calculated from the dxdy and the tiles loaded up until this frame.
 *
 * Example:
 * lambda = 3.7
 * Look at the hierarchy levels 3 (parent) and 4 (child). If the child is loaded (child_idx.w == 1) then save the
 * index positions from the parent and the child node; otherwise, check the levels above.
 */
vec4 traverse_idx_hierarchy(vec2 texture_coordinates)
{
    float lambda = -dxdy(texture_coordinates);

    float mix_ratio = fract(lambda);
    int desired_level = max(0, min(int(ceil(lambda)), int(max_level)));

    idx_tex_positions positions;

    uvec4 idx_child_pos = texture(hierarchical_idx_textures[desired_level], texture_coordinates).rgba;

    if(idx_child_pos.w == 1u)
    {
        uvec4 idx_parent_pos = texture(hierarchical_idx_textures[desired_level - 1], texture_coordinates).rgba;
        positions = idx_tex_positions(desired_level - 1, idx_parent_pos, desired_level, idx_child_pos);
    }
    else
    {
        /// Binary-like search for maximum available depth
        int left = 0;
        int right = desired_level;
        while(left < right)
        {
            int i = (left + right) / 2;

            uvec4 idx_child_pos = texture(hierarchical_idx_textures[i], texture_coordinates).rgba;

            if(i == 0)
            {
                positions = idx_tex_positions(0, idx_child_pos, 0, idx_child_pos);
                break;
            }

            if(idx_child_pos.w == 1u)
            {
                if(right - i == 1)
                {
                    uvec4 idx_parent_pos = texture(hierarchical_idx_textures[i - 1], texture_coordinates).rgba;
                    positions = idx_tex_positions(i - 1, idx_parent_pos, i, idx_child_pos);
                    break;
                }

                left = min(i, desired_level);
            }
            else
            {
                right = max(i, 0);
            }
        }
    }

    /*

    // Go from desired tree level downwards to root until a loaded tile is found
    for(int i = desired_level; i >= 0; --i)
    {
        uvec4 idx_child_pos = texture(hierarchical_idx_textures[i], texture_coordinates).rgba;

        // check if the requested tile is loaded and if we are not at the root level
        // enables to mix (linearly interpolate) between hierarchy levels
        if(idx_child_pos.w == 1 && i >= 1)
        {
            uvec4 idx_parent_pos = texture(hierarchical_idx_textures[i - 1], texture_coordinates).rgba;
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

    */

    vec4 c;
    if(toggle_visualization == 1 || toggle_visualization == 2)
    {
        c = illustrate_level(desired_level, mix_ratio, positions);
    }
    else
    {
        c = mix_colors(positions, texture_coordinates, mix_ratio);
    }

    if(enable_feedback)
    {
        if(int(gl_FragCoord.x) % COLUMN_SKIP == 0 && int(gl_FragCoord.y) % ROW_SKIP == 0)
        {
            update_feedback(desired_level, positions.child_idx);

#ifdef HIGHLIGHT_FEEDBACK_POINT
            c = vec4(1., 0., 0., 1.);
#endif
        }
    }

    return c;
}

void main()
{
    // swap y axis
    vec2 texture_coordinates = vec2(texture_coord);
    texture_coordinates.y = 1.0 - texture_coordinates.y;

    out_color = traverse_idx_hierarchy(texture_coordinates);
}