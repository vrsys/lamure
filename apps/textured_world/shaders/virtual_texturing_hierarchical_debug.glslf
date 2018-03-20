#version 440 core

in vec2 texture_coord;

flat in uint max_level;
flat in uvec2 physical_texture_dim;

flat in uint toggle_view;
flat in vec2 tile_size;
flat in vec2 tile_padding;

layout(binding = 0) uniform usampler2D hierarchical_idx_textures[16];
layout(binding = 17) uniform sampler2DArray physical_texture_array;



layout(std430, binding = 0) buffer out_feedback { int out_feedback_values[]; };

layout(location = 0) out vec4 out_color;

struct idx_tex_positions
{
    int parent_lvl;
    uvec4 parent_idx;
    int child_lvl;
    uvec4 child_idx;
};

float dxdy()
{
    int depth = 0;
    vec2 c = texture_coord * (pow(2, depth)) * tile_size;

    float rho = max(sqrt(pow(dFdx(c.x), 2) + pow(dFdx(c.y), 2)), sqrt(pow(dFdy(c.x), 2) + pow(dFdy(c.y), 2)));

    float lambda = log2(rho);

    return lambda;
}

idx_tex_positions choice_0(vec2 texture_coordinates)
{
    uvec4 idx_pos = texture(hierarchical_idx_textures[0], texture_coordinates).rgba;
    return idx_tex_positions(0, idx_pos, 0, idx_pos);
}

idx_tex_positions choice_1(vec2 texture_coordinates)
{
    uvec4 idx_child_pos = texture(hierarchical_idx_textures[1], texture_coordinates).rgba;
    uvec4 idx_parent_pos = texture(hierarchical_idx_textures[0], texture_coordinates).rgba;

    if(idx_child_pos.w != 0)
    { // tile is available at level 1
        return idx_tex_positions(0, idx_parent_pos, 1, idx_child_pos);
    }
    else
    { // tile is not available at level 1
        return idx_tex_positions(0, idx_parent_pos, 0, idx_parent_pos);
    }
}

idx_tex_positions choice_2(vec2 texture_coordinates)
{
    uvec4 idx_child_pos = texture(hierarchical_idx_textures[2], texture_coordinates).rgba;
    uvec4 idx_parent_pos = texture(hierarchical_idx_textures[1], texture_coordinates).rgba;

    if(idx_child_pos.w != 0)
    { // tile is available at level 2
        return idx_tex_positions(1, idx_parent_pos, 2, idx_child_pos);
    }
    else
    {
        return choice_1(texture_coordinates);
    }
}

idx_tex_positions choice_3(vec2 texture_coordinates)
{
    uvec4 idx_child_pos = texture(hierarchical_idx_textures[3], texture_coordinates).rgba;
    uvec4 idx_parent_pos = texture(hierarchical_idx_textures[2], texture_coordinates).rgba;

    if(idx_child_pos.w != 0)
    { // tile is available at level 3
        return idx_tex_positions(2, idx_parent_pos, 3, idx_child_pos);
    }
    else
    {
        return choice_2(texture_coordinates);
    }
}

idx_tex_positions choice_4(vec2 texture_coordinates)
{
    uvec4 idx_child_pos = texture(hierarchical_idx_textures[4], texture_coordinates).rgba;
    uvec4 idx_parent_pos = texture(hierarchical_idx_textures[3], texture_coordinates).rgba;

    if(idx_child_pos.w != 0)
    { // tile is available at level 4
        return idx_tex_positions(3, idx_parent_pos, 4, idx_child_pos);
    }
    else
    {
        return choice_3(texture_coordinates);
    }
}

idx_tex_positions choice_5(vec2 texture_coordinates)
{
    uvec4 idx_child_pos = texture(hierarchical_idx_textures[5], texture_coordinates).rgba;
    uvec4 idx_parent_pos = texture(hierarchical_idx_textures[4], texture_coordinates).rgba;

    if(idx_child_pos.w != 0)
    { // tile is available at level 5
        return idx_tex_positions(4, idx_parent_pos, 5, idx_child_pos);
    }
    else
    {
        return choice_4(texture_coordinates);
    }
}

idx_tex_positions choice_6(vec2 texture_coordinates)
{
    uvec4 idx_child_pos = texture(hierarchical_idx_textures[6], texture_coordinates).rgba;
    uvec4 idx_parent_pos = texture(hierarchical_idx_textures[5], texture_coordinates).rgba;

    if(idx_child_pos.w != 0)
    { // tile is available at level 6
        return idx_tex_positions(5, idx_parent_pos, 6, idx_child_pos);
    }
    else
    {
        return choice_5(texture_coordinates);
    }
}

idx_tex_positions choice_7(vec2 texture_coordinates)
{
    uvec4 idx_child_pos = texture(hierarchical_idx_textures[7], texture_coordinates).rgba;
    uvec4 idx_parent_pos = texture(hierarchical_idx_textures[6], texture_coordinates).rgba;

    if(idx_child_pos.w != 0)
    { // tile is available at level 7
        return idx_tex_positions(6, idx_parent_pos, 7, idx_child_pos);
    }
    else
    {
        return choice_6(texture_coordinates);
    }
}

idx_tex_positions choice_8(vec2 texture_coordinates)
{
    uvec4 idx_child_pos = texture(hierarchical_idx_textures[8], texture_coordinates).rgba;
    uvec4 idx_parent_pos = texture(hierarchical_idx_textures[7], texture_coordinates).rgba;

    if(idx_child_pos.w != 0)
    { // tile is available at level 8
        return idx_tex_positions(7, idx_parent_pos, 8, idx_child_pos);
    }
    else
    {
        return choice_7(texture_coordinates);
    }
}

idx_tex_positions choice_9(vec2 texture_coordinates)
{
    uvec4 idx_child_pos = texture(hierarchical_idx_textures[9], texture_coordinates).rgba;
    uvec4 idx_parent_pos = texture(hierarchical_idx_textures[8], texture_coordinates).rgba;

    if(idx_child_pos.w != 0)
    { // tile is available at level 9
        return idx_tex_positions(8, idx_parent_pos, 9, idx_child_pos);
    }
    else
    {
        return choice_8(texture_coordinates);
    }
}

vec4 get_physical_texture_color(uvec4 index_quadruple, vec2 texture_sampling_coordinates, uint current_level)
{
    // exponent for calculating the occupied pixels in our index texture, based on which level the tile is in
    uint tile_occupation_exponent = max_level - current_level;

    // 2^tile_occupation_exponent defines how many pixel (of the index texture) are used by the given tile
    uint occupied_index_pixel_per_dimension = uint(1 << tile_occupation_exponent);

    // offset represented as tiles is divided by total num tiles per axis
    // (replace max_width_tiles later by correct uniform)
    // extracting x,y from index texture
    uvec2 base_xy_offset = index_quadruple.xy;

    // base x,y coordinates * number of tiles / number of used index texture pixel
    // taking the factional part by modf
    vec2 physical_tile_ratio_xy = fract(texture_sampling_coordinates * (1 << max_level) / vec2(occupied_index_pixel_per_dimension));

    // Use only tile_size - 2*tile_padding pixels to render scene
    // Therefore, scale reduced tile size to full size and translate it
    vec2 padding_scale = 1 - 2 * tile_padding / tile_size;
    vec2 padding_offset = tile_padding / tile_size;

    // adding the ratio for every texel to our base offset to get the right pixel in our tile
    // and dividing it by the dimension of the phy. tex.
    vec2 physical_texture_coordinates = (base_xy_offset.xy + physical_tile_ratio_xy * padding_scale + padding_offset) / physical_texture_dim;

    // outputting the calculated coordinate from our physical texture
    vec4 c = texture(physical_texture_array, vec3(physical_texture_coordinates, index_quadruple.z));

    //c = (c + vec4(0.0, 1.0 * (float(current_level) / max_level), float(index_quadruple.z) / 2, 1.0)) * 0.5;

    return c;
}

void update_feedback(int feedback_value, uvec4 base_offset)
{
    uint one_d_feedback_ssbo_index = base_offset.x + base_offset.y * physical_texture_dim.x + base_offset.z * physical_texture_dim.x * physical_texture_dim.y;

    atomicMax(out_feedback_values[one_d_feedback_ssbo_index], feedback_value);
}

vec4 traverse_idx_hierarchy(float lambda, vec2 texture_coordinates)
{
    idx_tex_positions positions;

    float mix_ratio = fract(lambda);
    int desired_level = int(ceil(lambda));

    if(desired_level <= 0)
    //if(true)
    {
        positions = choice_0(texture_coordinates);
    }
    else if(desired_level == 1)
    {
        positions = choice_1(texture_coordinates);
    }
    else if(desired_level == 2)
    {
        positions = choice_2(texture_coordinates);
    }
    else if(desired_level == 3)
    {
        positions = choice_3(texture_coordinates);
    }
    else if(desired_level == 4)
    {
        positions = choice_4(texture_coordinates);
    }
    else if(desired_level == 5)
    {
        positions = choice_5(texture_coordinates);
    }
    else if(desired_level == 6)
    {
        positions = choice_6(texture_coordinates);
    }
    else if(desired_level == 7)
    {
        positions = choice_7(texture_coordinates);
    }
    else if(desired_level == 8)
    {
        positions = choice_8(texture_coordinates);
    }
    else if(desired_level == 9)
    {
        positions = choice_9(texture_coordinates);
    }
    else
    {
        positions = choice_9(texture_coordinates);
    }

    vec4 c;
    if(positions.child_lvl == desired_level)
    {
        vec4 child_color = get_physical_texture_color(positions.child_idx, texture_coordinates, positions.child_lvl);
        vec4 parent_color = get_physical_texture_color(positions.parent_idx, texture_coordinates, positions.parent_lvl);

        // x * (1 - a) + y * a
        c = mix(parent_color, child_color, mix_ratio);
    }
    else
    {
        c = get_physical_texture_color(positions.child_idx, texture_coordinates, positions.child_lvl);
    }

    int feedback_value = desired_level;
    update_feedback(feedback_value, positions.child_idx);

    return c;
}

void main()
{
    // swap y axis
    vec2 texture_coordinates = vec2(texture_coord);
    texture_coordinates.y = 1.0 - texture_coordinates.y;
    vec4 c;

    float lambda = -dxdy();
    c = traverse_idx_hierarchy(lambda, texture_coordinates);

    out_color = c;
}