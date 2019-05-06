#ifndef LAMURE_MP_CONSTANTS_H
#define LAMURE_MP_CONSTANTS_H

/***
 * All app constants should be contained in this file (to reduce the clutter)
 *
 ***/

const std::string STRING_APP_NAME = "Lamure Mesh Preprocessing";

const float packing_scale = 400.f;
const double target_percentage_charts_with_enough_pixels = 0.90;
const bool want_raw_file = false;

const std::string vertex_shader_src = "#version 420\n\
    layout (location = 0) in vec2 vertex_old_coord;\n\
    layout (location = 1) in vec2 vertex_new_coord;\n\
    \n\
    uniform vec2 viewport_offset;\n\
    uniform vec2 viewport_scale;\n\
    \n\
    varying vec2 passed_uv;\n\
    \n\
    void main() {\n\
      vec2 coord = vec2(vertex_new_coord.x, vertex_new_coord.y);\n\
      vec2 coord_translated = coord - viewport_offset; \n\
      vec2 coord_scaled = coord_translated / viewport_scale; \n\
      gl_Position = vec4((coord_scaled-0.5)*2.0, 0.5, 1.0);\n\
      passed_uv = vertex_old_coord;\n\
    }";

const std::string fragment_shader_src = "#version 420\n\
    uniform sampler2D image;\n\
    varying vec2 passed_uv;\n\
    \n\
    layout (location = 0) out vec4 fragment_color;\n\
    \n\
    void main() {\n\
      vec4 color = texture(image, passed_uv).rgba;\n\
      fragment_color = vec4(color.rgb, 1.0);\n\
    }";

const std::string dilation_vertex_shader_src = "#version 420\n\
    layout (location = 0) in vec2 vertex_old_coord;\n\
    layout (location = 1) in vec2 vertex_new_coord;\n\
    \n\
    varying vec2 passed_uv;\n\
    \n\
    void main() {\n\
      vec2 coord = vec2(vertex_old_coord.x, vertex_old_coord.y);\n\
      gl_Position = vec4((coord), 0.5, 1.0);\n\
      passed_uv = vertex_new_coord;\n\
    }";

const std::string dilation_fragment_shader_src = "#version 420\n\
    uniform sampler2D image;\n\
    uniform int image_width;\n\
    uniform int image_height;\n\
    varying vec2 passed_uv;\n\
    \n\
    layout (location = 0) out vec4 fragment_color;\n\
    \n\
    vec4 weighted_dilation() {\n\
      vec4 fallback_color = vec4(1.0, 0.0, 1.0, 1.0);\n\
      vec4 accumulated_color = vec4(0.0, 0.0, 0.0, 1.0);\n\
      float accumulated_weight = 0.0;\n\
      for(int y_offset = -1; y_offset < 2; ++y_offset) {\n\
        if( 0 == y_offset) {\n\
          continue;\n\
        }\n\
        ivec2 sampling_frag_coord = ivec2(gl_FragCoord.xy) + ivec2(0, y_offset);\n\
\n\
        if( \n\
           sampling_frag_coord.y >= image_height || sampling_frag_coord.y < 0 ) {\n\
            continue;\n\
           }\n\
\n\
        vec3 sampled_color = texelFetch(image, ivec2(sampling_frag_coord), 0).rgb;\n\
\n\
        if( !(1.0 == sampled_color.r && 0.0 == sampled_color.g && 1.0 == sampled_color.b) ) {\n\
          ivec2 pixel_dist = ivec2(gl_FragCoord.xy) - sampling_frag_coord;\n\
          float eucl_distance = sqrt(pixel_dist.x * pixel_dist.x + pixel_dist.y * pixel_dist.y);\n\
          float weight = 1.0 / eucl_distance;\n\
          accumulated_color += vec4(sampled_color, 1.0) * weight;\n\
          accumulated_weight += weight;\n\
        }\n\
\n\
    }\n\
\n\
      for(int x_offset = -1; x_offset < 2; ++x_offset) {\n\
        if( 0 == x_offset) {\n\
          continue;\n\
        }\n\
        ivec2 sampling_frag_coord = ivec2(gl_FragCoord.xy) + ivec2(x_offset, 0);\n\
\n\
        if( \n\
           sampling_frag_coord.x >= image_width || sampling_frag_coord.x < 0 ) {\n\
            continue;\n\
           }\n\
\n\
        vec3 sampled_color = texelFetch(image, ivec2(sampling_frag_coord), 0).rgb;\n\
\n\
        if( !(1.0 == sampled_color.r && 0.0 == sampled_color.g && 1.0 == sampled_color.b) ) {\n\
          ivec2 pixel_dist = ivec2(gl_FragCoord.xy) - sampling_frag_coord;\n\
          float eucl_distance = sqrt(pixel_dist.x * pixel_dist.x + pixel_dist.y * pixel_dist.y);\n\
          float weight = 1.0 / eucl_distance;\n\
          accumulated_color += vec4(sampled_color, 1.0) * weight;\n\
          accumulated_weight += weight;\n\
        }\n\
\n\
    }\n\
\n\
\n\
      if(accumulated_weight > 0.0) {\n\
        //return vec4(0.0, 1.0, 0.0, 1.0);\n\
        return vec4( (accumulated_color / accumulated_weight).rgb, 1.0);\n\
      } else {\n\
        return fallback_color;\n\
      }\n\
    }\n\
\n\
    void main() {\n\
      vec4 color = texture(image, passed_uv).rgba;\n\
    \n\
      if(1.0 == color.r && 0.0 == color.g && 1.0 == color.b) {\n\
      \n\
        fragment_color = weighted_dilation();\n\
       //fragment_color = vec4(1.0, 0.0, 0.0, 1.0); \n\
      } else {\n\
        fragment_color = texelFetch(image, ivec2(gl_FragCoord.xy), 0);\n\
      }\n\
    }";

#endif // LAMURE_MP_CONSTANTS_H
