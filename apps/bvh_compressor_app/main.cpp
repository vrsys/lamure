// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <lamure/ren/model_database.h>
#include <lamure/bounding_box.h>

#include <lamure/types.h>

//for convenient access to the node attributes
#include <lamure/ren/bvh.h>
#include <lamure/ren/bvh_stream.h>
#include <lamure/ren/lod_stream.h>

// for rewriting of changed attributes (primitive size, avg radius deviation if needed)
#include <lamure/pre/bvh.h>
#include <lamure/pre/bvh_stream.h>


#include "file_handler.h"




char* get_cmd_option(char** begin, char** end, const std::string & option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}


struct surfel {
  float pos[3];
  uint8_t rgbf[4];
  float size;
  float normal[3];
};

struct quantized_surfel {
  uint16_t pos_16ui_components[3];
  uint16_t normal_16ui;
  uint32_t color777ui_and_radius11ui_combined;
  //uint16_t color_565ui_combined;
  //uint16_t  radius_scale;
};

void quantize_position(surfel const& in_uncompressed_surfel, quantized_surfel& out_compressed_surfel, scm::gl::boxf const& node_extents) {
  double quantization_steps[3] = {0.0, 0.0, 0.0};
  double range_per_axis[3] = {0.0, 0.0, 0.0};

  int64_t max_position_quantization_index = std::pow(2,16);
  for(int dim_idx = 0; dim_idx < 3; ++dim_idx) {

    range_per_axis[dim_idx] = (((double)node_extents.max_vertex()[dim_idx]) - ((double)node_extents.min_vertex()[dim_idx]));
    quantization_steps[dim_idx] = range_per_axis[dim_idx] / max_position_quantization_index;

    //double normalized_position = (in_uncompressed_surfel.pos[dim_idx] - ((double)node_extents.min_vertex()[dim_idx]) )  / range_per_axis[dim_idx];

    int32_t overflow_protected_pos = 0;

    overflow_protected_pos = int32_t(std::round( ((in_uncompressed_surfel.pos[dim_idx] - ((double)node_extents.min_vertex()[dim_idx]) )  / quantization_steps[dim_idx] ) ) );

    out_compressed_surfel.pos_16ui_components[dim_idx] = std::min(int32_t(std::numeric_limits<uint16_t>::max()), std::max(int32_t(std::numeric_limits<uint16_t>::min()), overflow_protected_pos));
  }
}
/*
void unquantize_position(quantized_surfel const& in_compressed_surfel, surfel& out_uncompressed_surfel, scm::gl::boxf const& node_extents) {
  unquantized_pos[dim_idx] = qz_surfel.pos_16ui_components[dim_idx] * ((((double)bvh_bounding_boxes[node_idx].max_vertex()[dim_idx]) - ((double)bvh_bounding_boxes[node_idx].min_vertex()[dim_idx])) / 65536.0 ) + ((double)bvh_bounding_boxes[node_idx].min_vertex()[dim_idx] );
}
*/

void quantize_radius(surfel const& in_uncompressed_surfel, quantized_surfel& out_compressed_surfel, float const avg_surfel_radius, float const max_surfel_radius_deviation) {
  //currently, min and max radius are not exposed in the tree, so we should use the avg radius
  //to not introduce large errors, we need to calculate the range from avg rad to min and max rad and use the max range to quantize (this does not filter outliers)

  if(max_surfel_radius_deviation < 0.0 ) {
    std::cout << "MAX SURFEL RADIUS DEVIATION SMALLER THAN ZERO\n";
  }

  //std::cout << std::setprecision(16) << max_surfel_radius_deviation << "\n";
  double one_sided_quantization_step = 0.0;
  uint32_t num_radius_bit = 11;
  int64_t max_quantization_idx = std::pow(2, num_radius_bit);
  int half_range_minus_one = max_quantization_idx / 2 - 1;

  one_sided_quantization_step = ((double)max_surfel_radius_deviation) / half_range_minus_one;

  if(  (0.0 == in_uncompressed_surfel.size) ) { //find surfel with radius 0 and set them to the invalid value (max_quantization_idx - 1)

    out_compressed_surfel.color777ui_and_radius11ui_combined = (out_compressed_surfel.color777ui_and_radius11ui_combined & (0xFFFFF800)) | ( max_quantization_idx - 1 ); //(max_quantization index - 1) marks invalid surfels (with radius zero)  (=> 255 for 8 bit, 65535 for 16 bit)
    return;
  } else if ((0.0 == one_sided_quantization_step) ) { //if it was not a zero surfel, make sure, handle the division by zero case (=> all surfels have exactly the same size)
    out_compressed_surfel.color777ui_and_radius11ui_combined = (out_compressed_surfel.color777ui_and_radius11ui_combined & (0xFFFFF800)) | ( half_range_minus_one );
    return;
  } // if it was not a zero surfel, handle

  if ((0.0 == one_sided_quantization_step) ||)

  double reference_min_range = avg_surfel_radius - max_surfel_radius_deviation;
  double normalized_float_radius = ((double)in_uncompressed_surfel.size - reference_min_range) / (2*max_surfel_radius_deviation);

  int32_t overflow_protected_rad = std::round(normalized_float_radius * half_range_minus_one * 2); //skip 1 out of 256 steps for convenience

  uint32_t quantized_radius_to_write = std::min( uint32_t(half_range_minus_one*2), std::max( uint32_t(0), uint32_t(overflow_protected_rad) ) );
  out_compressed_surfel.color777ui_and_radius11ui_combined =  ((out_compressed_surfel.color777ui_and_radius11ui_combined & (0xFFFFF800)) | quantized_radius_to_write );
}

void quantize_color(surfel const& in_uncompressed_surfel, quantized_surfel& out_compressed_surfel) {
  /*
  int32_t rb_quantization_step = 8;
  int32_t g_quantization_step  = 4;

  int8_t r5 = int32_t( std::round(in_uncompressed_surfel.rgbf[0] / (double)rb_quantization_step) );
  int8_t g6 = int32_t( std::round(in_uncompressed_surfel.rgbf[1] /  (double)g_quantization_step) );
  int8_t b5 = int32_t( std::round(in_uncompressed_surfel.rgbf[2] / (double)rb_quantization_step) );

  r5 = std::min(int8_t(31), r5);
  g6 = std::min(int8_t(63), g6);
  b5 = std::min(int8_t(31), b5);

  uint8_t rb_combination_mask = 0x1F;
  uint8_t  g_combination_mask = 0x3F;


  //bit pattern: rrrrr gggggg bbbbb  <----- 16 bit
  uint16_t quantized_and_combined_color =  ((r5 & rb_combination_mask) << 11) |
                                           ((g6 &  g_combination_mask) <<  5) |
                                           ((b5 & rb_combination_mask) << 0);



  out_compressed_surfel.color_565ui_combined = quantized_and_combined_color;
  */

  int32_t rb_quantization_step = 2;
  int32_t g_quantization_step  = 2;

  int16_t r7 = int32_t( std::round(in_uncompressed_surfel.rgbf[0] / (double)rb_quantization_step) );
  int16_t g7 = int32_t( std::round(in_uncompressed_surfel.rgbf[1] /  (double)g_quantization_step) );
  int16_t b7 = int32_t( std::round(in_uncompressed_surfel.rgbf[2] / (double)rb_quantization_step) );

  r7 = std::min(int16_t(127), r7);
  g7 = std::min(int16_t(127), g7);
  b7 = std::min(int16_t(127), b7);

  uint16_t rb_combination_mask = 0x7F;
  uint16_t  g_combination_mask = 0x7F;


  //bit pattern: rrrrr gggggg bbbbb  <----- 16 bit
  uint32_t quantized_and_combined_color =  ((r7 & rb_combination_mask) << 14) |
                                           ((g7 &  g_combination_mask) <<  7) |
                                           ((b7 & rb_combination_mask) <<  0);

  out_compressed_surfel.color777ui_and_radius11ui_combined = (out_compressed_surfel.color777ui_and_radius11ui_combined & (0x7FF) ) | ( quantized_and_combined_color << 11 );
}

void quantize_normal(surfel const& in_uncompressed_surfel, quantized_surfel& out_compressed_surfel) {


  double max_abs_normal_component = -1.0; 
  int32_t dominant_axis_idx = -1;

  for(int32_t dim_idx = 0; dim_idx < 3; ++dim_idx) {
    double tmp_abs_normal_component = std::fabs(in_uncompressed_surfel.normal[dim_idx]);
    if(max_abs_normal_component < tmp_abs_normal_component) {
      max_abs_normal_component = tmp_abs_normal_component;
      dominant_axis_idx = dim_idx;
    }
  }


  //we use 104*105 normals per face (6*104*105 slightly < 2**16)
  int32_t face_positions_u = 104;
  int32_t face_positions_v = 105; 

  //int32_t face_positions_u = 147;
  //int32_t face_positions_v = 148;

  int32_t normal_positions_per_face = face_positions_u*face_positions_v;

  //face ids based on dominant axes: -x = 0; +x = 1; -y = 2; +y=3; -z = 4; +z = 5 
  int32_t dominant_face_idx = dominant_axis_idx*2 + ( in_uncompressed_surfel.normal[dominant_axis_idx] < 0.0 ? 1 : 0 );
  //int32_t dominant_face_idx = dominant_axis_idx;//+ ( in_uncompressed_surfel.normal[dominant_axis_idx] < 0.0 ? 1 : 0 );
  //quantize dom_axis_idx + 1 with 104 positions and dom_axis_idx+2 with 105 positions


  int32_t dominant_axis_idx_p1 = (dominant_axis_idx + 1) % 3;
  int32_t dominant_axis_idx_p2 = (dominant_axis_idx + 2) % 3;


  double normalized_face_component_u = ( ( (double) in_uncompressed_surfel.normal[dominant_axis_idx_p1]) - (-1.0) ) / 2.0;
  double normalized_face_component_v = ( ( (double) in_uncompressed_surfel.normal[dominant_axis_idx_p2]) - (-1.0) ) / 2.0;

  int32_t quantized_offset_u = std::min(face_positions_u, std::max(0, int32_t(std::round(normalized_face_component_u * face_positions_u) ) ) );
  int32_t quantized_offset_v = std::min(face_positions_v, std::max(0, int32_t(std::round(normalized_face_component_v * face_positions_v) ) ) );

  uint16_t quantized_normal_enumerator = dominant_face_idx * normal_positions_per_face + quantized_offset_v * face_positions_u + quantized_offset_u;


    out_compressed_surfel.normal_16ui = quantized_normal_enumerator;
}


//Assume normalized input on +Z hemisphere.
//Output is on [-1, 1].
void float32x3_to_hemioct(surfel const& in_uncompressed_surfel, quantized_surfel& out_compressed_surfel) {
//Project the hemisphere onto the hemi-octahedron,
//and then into the xy plane


  double tmp_normal[3] = {in_uncompressed_surfel.normal[0], in_uncompressed_surfel.normal[1], in_uncompressed_surfel.normal[2]};

  if(tmp_normal[2] < 0.0) {
    tmp_normal[0] *= -1;
    tmp_normal[1] *= -1;
    tmp_normal[2] *= -1;
  }

double p[2];
p[0] = tmp_normal[0] * (1.0 / ( std::fabs(tmp_normal[0]) + std::fabs(tmp_normal[1]) + tmp_normal[2] )  );
p[1] = tmp_normal[1] * (1.0 / ( std::fabs(tmp_normal[0]) + std::fabs(tmp_normal[1]) + tmp_normal[2] )  );


uint16_t compressed_first_part_8_bit = ( std::max(0, std::min(255, int32_t(std::round((((p[0] + p[1]) + 1.0) / 2.0) * 255)) ) )) << 8 ;
uint16_t compressed_second_part_8_bit = ( std::max(0, std::min(255, int32_t(std::round((((p[0] - p[1]) + 1.0) / 2.0) * 255)) ) )) << 0 ;

out_compressed_surfel.normal_16ui = compressed_first_part_8_bit | compressed_second_part_8_bit;
//Rotate and scale the center diamond to the unit square
//return vec2(p.x + p.y, p.x - p.y);
}



void hemioct_to_float32x3(quantized_surfel const& in_compressed_surfel, surfel& out_uncompressed_surfel) {

  uint16_t looked_up_compressed_normal = in_compressed_surfel.normal_16ui;

  double e[2] = {0.0, 0.0};

  e[0] = (((looked_up_compressed_normal >> 8) / 255.0) - 1.0) * 2.0;
  e[1] = (((looked_up_compressed_normal & 0xFF) / 255.0) - 1.0) * 2.0;

  //double tmp_compr_normal[2] = {in_uncompressed_surfel.normal[0], in_uncompressed_surfel.normal[1], in_uncompressed_surfel.normal[2]};
  double temp[2];
  temp[0] = (e[0] + e[1]) * 0.5;
  temp[1] = (e[0] - e[1]) * 0.5;

  out_uncompressed_surfel.normal[0] = temp[0];
  out_uncompressed_surfel.normal[1] = temp[1];
  out_uncompressed_surfel.normal[2] = 1.0 - std::fabs(temp[0]) - std::fabs(temp[1]);


  double vector_length = std::sqrt(out_uncompressed_surfel.normal[0]*out_uncompressed_surfel.normal[0] +
                                   out_uncompressed_surfel.normal[1]*out_uncompressed_surfel.normal[1] +
                                   out_uncompressed_surfel.normal[2]*out_uncompressed_surfel.normal[2] );


  out_uncompressed_surfel.normal[0] /= vector_length;
  out_uncompressed_surfel.normal[1] /= vector_length;
  out_uncompressed_surfel.normal[2] /= vector_length;
  // //TODO
  //vec3 v = vec3(temp, 1.0 - abs(temp.x) - abs(temp.y));
  //return normalize(v);
}

/*
vec3 hemioct_to_float32x3(vec2 e) {
//Rotate and scale the unit square back to the center diamond
vec2 temp = vec2(e.x + e.y, e.x - e.y) * 0.5;
vec3 v = vec3(temp, 1.0 - abs(temp.x) - abs(temp.y));
return normalize(v);
}
*/


void quantize_complete_surfel(surfel const& in_uncompressed_surfel, quantized_surfel& out_compressed_surfel, scm::gl::boxf const& node_extents, float const avg_surfel_radius, float const max_surfel_radius_deviation ) {
  quantize_position(in_uncompressed_surfel, out_compressed_surfel, node_extents);
  quantize_radius(in_uncompressed_surfel, out_compressed_surfel, avg_surfel_radius, max_surfel_radius_deviation);
  quantize_color(in_uncompressed_surfel, out_compressed_surfel);
  quantize_normal(in_uncompressed_surfel, out_compressed_surfel);
}

int main(int argc, char *argv[]) {

    if (argc == 1 ||
        cmd_option_exists(argv, argv+argc, "-h") ||
        !cmd_option_exists(argv, argv+argc, "-f")) {
        std::cout << "Usage: " << argv[0] << " <flags> -f <input_file>\n" <<
            "INFO: "<< argv[0] <<"\n" <<
            "\t-f: selects (uncompressed) .bvh input file\n" <<
            "\t    (-f flag is required)\n" <<
            "\tOUTPUT: Quantized *.bvhqz and corresponding *.lodqz files.\n" << 
            std::endl;
        return 0;
    }

    std::string input_uncompressed_bvh_file_name = std::string(get_cmd_option(argv, argv + argc, "-f"));

    std::string input_base_name = input_uncompressed_bvh_file_name.substr(0, input_uncompressed_bvh_file_name.size()-3);
    std::string input_bvh_ext   = input_uncompressed_bvh_file_name.substr(input_uncompressed_bvh_file_name.size()-3);
    if (input_bvh_ext.compare("bvh") != 0) {
        std::cout << "please specify an (uncompressed).bvh file as input" << std::endl;
        return 0;
    }

    std::string input_uncompressed_lod_file_name = input_base_name + "lod";

    std::cout << "Base filename: " << input_base_name << "\n";
    std::cout << "Extension: " << input_bvh_ext << "\n";


    std::string output_prefix = input_base_name;
    
    std::string out_bvhqz_file = output_prefix + "bvhqz";
    std::string out_lodqz_file = output_prefix + "lodqz";

    std::cout << "quantizing entire lod hierarchy" << std::endl
              << "from: " << input_base_name + ".lod" << std::endl
              << "to: " << input_base_name + ".lodqz" << std::endl;
       


    lamure::ren::bvh* bvh = new lamure::ren::bvh(input_uncompressed_bvh_file_name);
    
    size_t depth = -1;
    if (depth > bvh->get_depth() || depth < 0) {
      depth = bvh->get_depth();
    }
    std::cout << "Starting to compress.. " << depth << std::endl;

    lamure::ren::lod_stream* in_access = new lamure::ren::lod_stream();
    in_access->open(input_uncompressed_lod_file_name);

    size_t size_of_node = (uint64_t)bvh->get_primitives_per_node() * sizeof(lamure::ren::dataset::serialized_surfel);

    std::vector<surfel>           surfels(bvh->get_primitives_per_node());
    std::vector<quantized_surfel> qz_surfels(bvh->get_primitives_per_node());

    lamure::node_t first_leaf = bvh->get_first_node_id_of_depth(depth);
    lamure::node_t num_leafs = bvh->get_length_of_depth(depth);

    std::ofstream out_stream;
    out_stream.open(out_lodqz_file, std::ios::out | std::ios::trunc);
    out_stream.close();

    //consider hidden translation
    //const scm::math::vec3f& translation = bvh->get_translation();

    uint64_t num_surfels_excluded = 0;
    

    auto const& bvh_bounding_boxes = bvh->get_bounding_boxes();

    int global_max_r_error = 0;
    int global_max_g_error = 0;
    int global_max_b_error = 0;

    double max_position_error_per_level = 0.0;
    double max_relative_radius_error = 0.0;
    double avg_relative_radius_error = 0.0;
    int64_t contribs_rel_rad_error = 0;
    double max_angle_error = 0.0;
    double hemioct_max_angle_error = 0.0;

    double max_rel_rad_error_surfel_0_rad = 0.0;
    double max_rel_rad_error_surfel_1_rad = 0.0;
    //iterate over all nodes


    std::ios::openmode mode = std::ios::out | std::ios::binary;
    out_stream.open(out_lodqz_file, mode);

    for (lamure::node_t node_idx = 0; node_idx < first_leaf + num_leafs; ++node_idx) {
      if(node_idx % 1000 == 0)
      std::cout << "Starting with: " << node_idx << "\r";
      std::cout.flush();
      auto const& avg_surfel_radius = bvh->get_avg_primitive_extent(node_idx);
      auto max_radius_deviation = bvh->get_max_surfel_radius_deviation(node_idx);

#ifdef VERBOSE
        if ((leaf_id-first_leaf) % 1000 == 0) {
            std::cout << leaf_id-first_leaf << " / " << num_leafs << " writing: " << out_lodqz_file << std::endl;
        }
#endif
        
        in_access->read((char*)&surfels[0], node_idx * size_of_node, size_of_node);


   
        std::string filestr;
        std::stringstream ss(filestr);
        

        double unquantized_pos[3];

        //recompute max_radius_deviation if it was not set (in order to be able to compress bvhs prev v1.1)
        if( 0.0 == max_radius_deviation ) {
          float max_radius = 0.0f;
          float min_radius = std::numeric_limits<float>::max();

          for ( auto const& current_surfel : surfels) {
            //quantized_surfel qz_surfel;
            //const surfel& current_surfel = surfels[surfel_idx];

            if( 0.0 < current_surfel.size ) {
              max_radius = std::max(max_radius, current_surfel.size);
              min_radius = std::min(min_radius, current_surfel.size);
            }

          }

          max_radius_deviation = std::max( std::fabs(max_radius - avg_surfel_radius), std::fabs( avg_surfel_radius - min_radius )  );
          bvh->set_max_surfel_radius_deviation(node_idx, max_radius_deviation);
        }

        #pragma omp parallel for
        for (unsigned int i = 0; i < bvh->get_primitives_per_node(); ++i) {
            //quantized_surfel qz_surfel;
            const surfel& s = surfels[i];

            quantized_surfel& qz_surfel = qz_surfels[i];

            //quantization
            quantize_complete_surfel(s, qz_surfel, bvh_bounding_boxes[node_idx], avg_surfel_radius, max_radius_deviation);


            //quantization error measurement check
            double squared_pos_error_comps = 0.0;
            for(int dim_idx = 0; dim_idx < 3; ++dim_idx) {
              unquantized_pos[dim_idx] = qz_surfel.pos_16ui_components[dim_idx] * ((((double)bvh_bounding_boxes[node_idx].max_vertex()[dim_idx]) - ((double)bvh_bounding_boxes[node_idx].min_vertex()[dim_idx])) / 65536.0 ) + ((double)bvh_bounding_boxes[node_idx].min_vertex()[dim_idx] );
              squared_pos_error_comps += (unquantized_pos[dim_idx] - s.pos[dim_idx])*(unquantized_pos[dim_idx] - s.pos[dim_idx]);
            }

            double pos_quant_error = std::sqrt(squared_pos_error_comps);
            
/*
            std::cout << "Quantized surfel position: " << s.pos[0] << ", " << s.pos[1] << ", " << s.pos[2] << " to: " << unquantized_pos[0] << ", " << unquantized_pos[1] << ", " << unquantized_pos[2] << "\n";
            std::cout << "Pos Quantization Error: " << pos_quant_error << "\n";
*/


            //quantization error measurement check
            double radius_quantization_error = 0.0;
            int64_t max_quantization_idx = std::pow(2, 11);
            int64_t quantization_divisor = max_quantization_idx - 2;


            float min_radius = ((double)avg_surfel_radius - max_radius_deviation);
            float max_radius = ((double) ( max_radius_deviation + avg_surfel_radius ) );

            float unquantized_radius = (qz_surfel.color777ui_and_radius11ui_combined & (0x7FF )) * ( ( max_radius - min_radius) / quantization_divisor ) + min_radius;
            radius_quantization_error = std::abs(s.size - unquantized_radius);
            
            
            //std::cout << "radii: " << s.size << "\t" << unquantized_radius << "\n";

            if(s.size > 0.0) {
              max_position_error_per_level = std::max(max_position_error_per_level, pos_quant_error);

          
              double denom_rad = std::max(s.size, unquantized_radius);
              double enum_val = std::fabs(s.size - unquantized_radius);

              double rad_error = enum_val / denom_rad;
              avg_relative_radius_error += rad_error;
              ++contribs_rel_rad_error;
              if(rad_error > max_relative_radius_error) {
                max_relative_radius_error = rad_error;

                max_rel_rad_error_surfel_0_rad = s.size;
                max_rel_rad_error_surfel_1_rad = unquantized_radius;

              }
            }
            
            int r_error = 0;
            int g_error = 0;
            int b_error = 0;

            int32_t unquantized_r = (0x7F & (qz_surfel.color777ui_and_radius11ui_combined >> 25) ) * 2;
            int32_t unquantized_g = (0x7F & (qz_surfel.color777ui_and_radius11ui_combined >> 18 ) ) * 2;
            int32_t unquantized_b = (0x7F & (qz_surfel.color777ui_and_radius11ui_combined >> 11 ) ) * 2;           

            r_error = std::abs(unquantized_r - s.rgbf[0]); 
            g_error = std::abs(unquantized_g - s.rgbf[1]); 
            b_error = std::abs(unquantized_b - s.rgbf[2]); 

            global_max_r_error = std::max(global_max_r_error, r_error);
            global_max_g_error = std::max(global_max_g_error, g_error);
            global_max_b_error = std::max(global_max_b_error, b_error);            
/*
            std::cout << "Real Red (quantized Red): " << (int32_t)(s.rgbf[0]) << "("<< unquantized_r << ")" << "\n";
            std::cout << "Real Green (quantized Green): " << (int32_t)(s.rgbf[1]) << "("<< unquantized_g << ")" << "\n";
            std::cout << "Real Blue (quantized Blue): " << (int32_t)(s.rgbf[2]) << "("<< unquantized_b << ")" << "\n";
*/

            int32_t num_points_u = 104;
            int32_t num_points_v = 105;
            int32_t face_divisor = 2;
/*
            int32_t num_points_u = 147;
            int32_t num_points_v = 148;
            int32_t face_divisor = 1;
*/
            float unquantized_normal[3] = {0.0, 0.0, 0.0};

            uint16_t compressed_normal_enumerator = qz_surfel.normal_16ui;

            //uint32_t face_id = compressed_normal_enumerator / (104*105);
            uint32_t face_id = compressed_normal_enumerator / (num_points_u * num_points_v);

            int8_t is_main_axis_negative = (face_id % 2) == 1 ? -1 : 1 ;
            compressed_normal_enumerator -= face_id * ((num_points_u * num_points_v));
            uint32_t v_component = compressed_normal_enumerator / num_points_u;
            uint32_t u_component = compressed_normal_enumerator % num_points_u;

            uint32_t main_axis = face_id / face_divisor;

            uint32_t first_comp_axis = (main_axis + 1) % 3;
            uint32_t second_comp_axis = (main_axis + 2) % 3;

            float first_component = (u_component / (float)(num_points_u) ) * 2.0 - 1.0;
            float second_component = (v_component / (float)(num_points_v) ) * 2.0 - 1.0;

            unquantized_normal[first_comp_axis] = first_component;
            unquantized_normal[second_comp_axis] = second_component;

            unquantized_normal[main_axis] = is_main_axis_negative * std::sqrt(-(first_component*first_component) - (second_component*second_component) + 1);


            double unquantized_normal_length = std::sqrt(unquantized_normal[0]*unquantized_normal[0] + unquantized_normal[1]*unquantized_normal[1]  + unquantized_normal[2]*unquantized_normal[2]  );

            for( int dim_idx = 0; dim_idx < 3; ++dim_idx ) {
              unquantized_normal[dim_idx] /= unquantized_normal_length;
            }

/*
            std::cout << "Original Normal: " << s.normal[0] << ", " << s.normal[1] << ", " << s.normal[2]<< "\n";
            std::cout << "Unquantized Normal: " << unquantized_normal[0] << ", " << unquantized_normal[1] << ", " << unquantized_normal[2] << "\n";
*/
            double angle_error = 180.0 * std::acos(unquantized_normal[0] * s.normal[0] + unquantized_normal[1] * s.normal[1] + unquantized_normal[2] * s.normal[2] ) / 3.14159265359;

            if(s.size > 0.0) {
              max_angle_error = std::max(angle_error, max_angle_error);
            }
            
//            std::cout << "angle error (in deg): " << angle_error << "\n";



/*

            float32x3_to_hemioct(s, qz_surfel);

            surfel ref_surfel;
            hemioct_to_float32x3(qz_surfel, ref_surfel);

            surfel flipped_z_surfel = s;

            if(flipped_z_surfel.normal[2] < 0.0) {
              flipped_z_surfel.normal[0] *= -1.0;
              flipped_z_surfel.normal[1] *= -1.0;
              flipped_z_surfel.normal[2] *= -1.0;
            }

            std::cout << "Hemioct Normal: " << ref_surfel.normal[0] << ", " << ref_surfel.normal[1] << ", " << ref_surfel.normal[2] << "\n";

            double hemioct_angle_error = 180.0 * std::acos(ref_surfel.normal[0] * flipped_z_surfel.normal[0] + ref_surfel.normal[1] * flipped_z_surfel.normal[1] + ref_surfel.normal[2] * flipped_z_surfel.normal[2] ) / 3.14159265359;

            hemioct_max_angle_error = std::max(hemioct_angle_error, hemioct_max_angle_error);
*/

 

          //out_stream.write((char*) )

        }

        out_stream.write((char*) &qz_surfels[0], bvh->get_primitives_per_node() * sizeof(quantized_surfel) );



    }
    out_stream.close();

       
    std::cout << "Writing!!\n";
    std::cout << "Global max r g b error: " << global_max_r_error << ", " << global_max_g_error << ", " << global_max_b_error << "\n";
    std::cout << "Max angular error: " << max_angle_error << "\n";
    std::cout << "Max hemioct angular error: " << hemioct_max_angle_error << "\n";

    std::cout << "Max relative radius error: " << max_relative_radius_error << "\n";
    std::cout << "Corresponding radii: " << max_rel_rad_error_surfel_0_rad << ", " << max_rel_rad_error_surfel_1_rad << "\n";
    std::cout << "Max position error: " << max_position_error_per_level << "\n";

    if( contribs_rel_rad_error ) {

       avg_relative_radius_error /= contribs_rel_rad_error;
    }


    bvh->set_size_of_primitive( sizeof(lamure::ren::dataset::serialized_surfel_qz) );
    bvh->set_primitive(lamure::ren::bvh::primitive_type::POINTCLOUD_QZ);

    lamure::ren::bvh_stream bvh_ofstream;
    bvh_ofstream.write_bvh(out_bvhqz_file, *bvh);

    std::cout << "AVG RAD ERROR: " << avg_relative_radius_error << "\n";

    std::cout << "done. (" << num_surfels_excluded << " surfels excluded)" << std::endl;

    delete in_access;
    delete bvh;


}
