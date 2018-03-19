#version 440 core

in vec2 texture_coord;

flat in uint max_level_color;

// TODO: dynamic dimensions
flat in uvec2 index_dim_color;
flat in uvec2 physical_texture_dim;

flat in uint toggle_view;
flat in vec2 tile_size;
flat in vec2 tile_padding;

layout(binding = 0) uniform sampler2DArray physical_texture_array;
layout(binding = 1) uniform usampler2D index_color;

layout(binding = 3) uniform usampler2D idx_texture_0;
layout(binding = 4) uniform usampler2D idx_texture_1;
layout(binding = 5) uniform usampler2D idx_texture_2;
layout(binding = 6) uniform usampler2D idx_texture_3;
layout(binding = 7) uniform usampler2D idx_texture_4;
layout(binding = 8) uniform usampler2D idx_texture_5;
layout(binding = 9) uniform usampler2D idx_texture_6;
layout(binding = 10) uniform usampler2D idx_texture_7;

layout(std430, binding = 0) buffer out_feedback {
    int out_feedback_values[];
};

layout(location = 0) out vec4 out_color;

struct idx_tex_positions {
    int parent_lvl;
    uvec4 parent_idx;
    int child_lvl;
    uvec4 child_idx;
} ;

float dxdy(){
    int depth = 0;
    vec2 c = texture_coord * (pow(2, depth))*tile_size;

    float rho = max(sqrt( pow(dFdx(c.x), 2) + pow(dFdx(c.y), 2) ),
                    sqrt( pow(dFdy(c.x), 2) + pow(dFdy(c.y), 2) ));

    float lambda = log2(rho);

    return lambda;
}

idx_tex_positions choice_0(vec3 texture_coordinates) {
    uvec4 idx_pos = texture(idx_texture_0, texture_coordinates.xy).rgba;
    return idx_tex_positions(0, idx_pos, 0, idx_pos);
}

idx_tex_positions choice_1(vec3 texture_coordinates) {
    uvec4 idx_child_pos  = texture(idx_texture_1, texture_coordinates.xy).rgba;
    uvec4 idx_parent_pos = texture(idx_texture_0, texture_coordinates.xy).rgba;

    if(idx_child_pos.w == 1) { // tile is available at level 1
        return idx_tex_positions(0, idx_parent_pos, 1, idx_child_pos);
    } else { // tile is not available at level 1
        return idx_tex_positions(0, idx_parent_pos, 0, idx_parent_pos);
    }
}

idx_tex_positions choice_2(vec3 texture_coordinates) {
    uvec4 idx_child_pos  = texture(idx_texture_2, texture_coordinates.xy).rgba;
    uvec4 idx_parent_pos = texture(idx_texture_1, texture_coordinates.xy).rgba;

    if(idx_child_pos.w == 1) { // tile is available at level 2
        return idx_tex_positions(1, idx_parent_pos, 2, idx_child_pos);
    }
    else {
        return choice_1(texture_coordinates);
    }
}

idx_tex_positions choice_3(vec3 texture_coordinates) {
    uvec4 idx_child_pos  = texture(idx_texture_3, texture_coordinates.xy).rgba;
    uvec4 idx_parent_pos = texture(idx_texture_2, texture_coordinates.xy).rgba;

    if(idx_child_pos.w == 1) { // tile is available at level 3
        return idx_tex_positions(2, idx_parent_pos, 3, idx_child_pos);
    }
    else {
        return choice_2(texture_coordinates);
    }
}

idx_tex_positions choice_4(vec3 texture_coordinates) {
    uvec4 idx_child_pos  = texture(idx_texture_4, texture_coordinates.xy).rgba;
    uvec4 idx_parent_pos = texture(idx_texture_3, texture_coordinates.xy).rgba;

    if(idx_child_pos.w == 1) { // tile is available at level 4
        return idx_tex_positions(3, idx_parent_pos, 4, idx_child_pos);
    }
    else {
        return choice_3(texture_coordinates);
    }
}

idx_tex_positions choice_5(vec3 texture_coordinates) {
    uvec4 idx_child_pos  = texture(idx_texture_5, texture_coordinates.xy).rgba;
    uvec4 idx_parent_pos = texture(idx_texture_4, texture_coordinates.xy).rgba;

    if(idx_child_pos.w == 1) { // tile is available at level 5
        return idx_tex_positions(4, idx_parent_pos, 5, idx_child_pos);
    }
    else {
        return choice_4(texture_coordinates);
    }
}

idx_tex_positions choice_6(vec3 texture_coordinates) {
    uvec4 idx_child_pos  = texture(idx_texture_6, texture_coordinates.xy).rgba;
    uvec4 idx_parent_pos = texture(idx_texture_5, texture_coordinates.xy).rgba;

    if(idx_child_pos.w == 1) { // tile is available at level 6
        return idx_tex_positions(5, idx_parent_pos, 6, idx_child_pos);
    }
    else {
        return choice_5(texture_coordinates);
    }
}

idx_tex_positions choice_7(vec3 texture_coordinates) {
    uvec4 idx_child_pos  = texture(idx_texture_7, texture_coordinates.xy).rgba;
    uvec4 idx_parent_pos = texture(idx_texture_6, texture_coordinates.xy).rgba;

    if(idx_child_pos.w == 1) { // tile is available at level 7
        return idx_tex_positions(6, idx_parent_pos, 7, idx_child_pos);
    }
    else {
        return choice_6(texture_coordinates);
    }
}

vec4 get_physical_texture_color(uvec4 index_quadruple, vec3 texture_coordinates, int current_level){

#if 1
    // exponent for calculating the occupied pixels in our index texture, based on which level the tile is in
    uint tile_occupation_exponent = max_level_color - current_level;

    // 2^tile_occupation_exponent defines how many pixel (of the index texture) are used by the given tile
    uint occupied_index_pixel_per_dimension = uint(1 << tile_occupation_exponent);

    // offset represented as tiles is divided by total num tiles per axis
    // (replace max_width_tiles later by correct uniform)
    // extracting x,y from index texture
    uvec2 base_xy_offset = index_quadruple.xy;

    // base x,y coordinates * number of tiles / number of used index texture pixel
    // taking the factional part by modf
    vec2 physical_tile_ratio_xy = fract(texture_coordinates.xy * (1 << max_level_color) / vec2(occupied_index_pixel_per_dimension));

    // Use only tile_size - 2*tile_padding pixels to render scene
    // Therefore, scale reduced tile size to full size and translate it
    vec2 padding_scale = 1 - 2 * tile_padding / tile_size;
    vec2 padding_offset = tile_padding / tile_size;

    // adding the ratio for every texel to our base offset to get the right pixel in our tile
    // and dividing it by the dimension of the phy. tex.
    vec2 physical_texture_coordinates = (base_xy_offset.xy + physical_tile_ratio_xy * padding_scale + padding_offset) / physical_texture_dim;

//    if(current_level >= 1) {
//        return vec4(1,0,0,1);
//    }

    // outputting the calculated coordinate from our physical texture
    return texture(physical_texture_array, vec3(physical_texture_coordinates, index_quadruple.z));

#else
    uvec2 base_xy_offset = index_quadruple.xy;

    // Use only tile_size - 2*tile_padding pixels to render scene
    // Therefore, scale reduced tile size to full size and translate it
    vec2 padding_scale = 1 - 2 * tile_padding / tile_size;
    vec2 padding_offset = tile_padding / tile_size;

    // TODO: Not sure if this calculaten is correct
    vec2 physical_texture_coordinates = (base_xy_offset.xy + texture_coordinates.xy * padding_scale + padding_offset) / physical_texture_dim;

    // outputting the calculated coordinate from our physical texture
    // TODO: change it later to index_quadruple.z when new idx_layout is applied!
    return texture(physical_texture_array, vec3(physical_texture_coordinates, index_quadruple.z));

#endif
}

void update_feedback(int feedback_value, uvec4 base_offset) {
    uint one_d_feedback_ssbo_index = base_offset.x +
                                     base_offset.y * physical_texture_dim.x +
                                     base_offset.z * physical_texture_dim.x * physical_texture_dim.y;


#if 0
    atomicMax(out_feedback_values[one_d_feedback_ssbo_index], feedback_value);

#else
    atomicMax(out_feedback_values[0], 500000);
    atomicMax(out_feedback_values[1], 500000);
    atomicMax(out_feedback_values[2], 25000);
    atomicMax(out_feedback_values[3], 25000);
    atomicMax(out_feedback_values[4], 25000);
    atomicMax(out_feedback_values[5], 25000);
    atomicMax(out_feedback_values[6], 25000);
    atomicMax(out_feedback_values[7], 25000);
    atomicMax(out_feedback_values[8], 25000);
    atomicMax(out_feedback_values[9], 25000);
#endif
}

vec4 traverse_idx_hierarchy(float lambda, vec3 texture_coordinates) {
    idx_tex_positions positions;

    float mix_ratio = fract(lambda);
    int   desired_level = int(ceil(lambda));

    vec4 tmp_c = vec4(0,0,0,1);

    if(desired_level <= 0) {
        positions = choice_0(texture_coordinates);
        tmp_c = vec4(0,0,0,1);
    }

    else if(desired_level == 1) {
        positions = choice_1(texture_coordinates);
        tmp_c = vec4(1,0,0,1);
    }

    else if(desired_level == 2) {
        positions = choice_2(texture_coordinates);
        tmp_c = vec4(0,1,0,1);
    }

    else if(desired_level == 3) {
        positions = choice_3(texture_coordinates);
        tmp_c = vec4(0,0,1,1);
    }

    else if(desired_level == 4) {
        positions = choice_4(texture_coordinates);
        tmp_c = vec4(1,1,0,1);
    }

    else if(desired_level == 5) {
        positions = choice_5(texture_coordinates);
        tmp_c = vec4(1,0,1,1);
    }

    else if(desired_level == 6) {
        positions = choice_6(texture_coordinates);
        tmp_c = vec4(0,1,1,1);
    }

    else { // desired_level >= 7
        positions = choice_7(texture_coordinates);
        tmp_c = vec4(1,1,1,1);
    }

    vec4 c;
    if(positions.child_lvl == desired_level) {
        vec4 child_color  = get_physical_texture_color(positions.child_idx, texture_coordinates, positions.child_lvl);
        vec4 parent_color = get_physical_texture_color(positions.parent_idx, texture_coordinates, positions.parent_lvl);

        // x * (1 - a) + y * a
        c = mix(parent_color, child_color, mix_ratio);
    } else {
        c = get_physical_texture_color(positions.child_idx, texture_coordinates, positions.child_lvl);
    }

    int feedback_value = desired_level - positions.child_lvl;
    update_feedback(feedback_value, positions.child_idx);
    atomicMax(out_feedback_values[10], desired_level);

//    return mix(vec4(0,0,0,1), tmp_c, mix_ratio);
    return c;
}

void main()
{
    // swap y axis
    vec3 texture_coordinates = vec3(texture_coord, 0.0);
    texture_coordinates.y = 1.0 - texture_coordinates.y;
    vec4 c;

#if 0
    uvec4 index_quadruple = texture(index_color, texture_coordinates.xy).rgba;
    texture_coordinates.z = index_quadruple.w;

    if(toggle_view == 0)
    { // Show the physical texture
        c = texture(physical_texture_array, texture_coordinates);
        out_color = c;
    }
    else
    { // Show the image viewer

        uint current_level = index_quadruple.z;

        // exponent for calculating the occupied pixels in our index texture, based on which level the tile is in
        uint tile_occupation_exponent = max_level_color - current_level;

        // 2^tile_occupation_exponent defines how many pixel (of the index texture) are used by the given tile
        uint occupied_index_pixel_per_dimension = uint(1 << tile_occupation_exponent);

        // offset represented as tiles is divided by total num tiles per axis
        // (replace max_width_tiles later by correct uniform)
        // extracting x,y from index texture
        uvec2 base_xy_offset = index_quadruple.xy;

        // base x,y coordinates * number of tiles / number of used index texture pixel
        // taking the factional part by modf
        vec2 physical_tile_ratio_xy = fract(texture_coordinates.xy * index_dim_color / vec2(occupied_index_pixel_per_dimension));

        // Use only tile_size - 2*tile_padding pixels to render scene
        // Therefore, scale reduced tile size to full size and translate it
        vec2 padding_scale = 1 - 2 * tile_padding / tile_size;
        vec2 padding_offset = tile_padding / tile_size;

        // adding the ratio for every texel to our base offset to get the right pixel in our tile
        // and dividing it by the dimension of the phy. tex.
        vec2 physical_texture_coordinates = (base_xy_offset.xy + physical_tile_ratio_xy * padding_scale + padding_offset) / physical_texture_dim;

        // outputting the calculated coordinate from our physical texture
        c = texture(physical_texture_array, vec3(physical_texture_coordinates, texture_coordinates.z));

        // c = (c + vec4(0.0, 1.0 * (float(current_level) / max_level_color), float(texture_coordinates.z) / 2, 1.0)) * 0.5;

        // feedback calculation based on accumulated use of each rendered tile
        uint one_d_feedback_ssbo_index = base_xy_offset.x + base_xy_offset.y * physical_texture_dim.x + index_quadruple.w * physical_texture_dim.x * physical_texture_dim.y;
        atomicAdd(out_feedback_values[one_d_feedback_ssbo_index], 1);
    }
#else
        float lambda = -dxdy();
        c = traverse_idx_hierarchy(lambda, texture_coordinates);

//        if(lambda > 0) {
//            c = vec4((lambda) / 12.0, 0, 0, 1);
//        }
//        else if (lambda < 0) {
//            c = vec4(0,abs((lambda)) / 12.0, 0, 1);
//        }

#endif

    out_color = c;
}