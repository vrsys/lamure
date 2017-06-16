// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <lamure/ren/model_database.h>
#include <lamure/bounding_box.h>

#include <lamure/types.h>

#include <lamure/ren/bvh.h>
#include <lamure/ren/lod_stream.h>

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
  uint16_t color_565ui_combined;
  uint8_t  radius_scale;
};

void quantize_position(surfel const& in_uncompressed_surfel, quantized_surfel& out_compressed_surfel, scm::gl::boxf const& node_extents) {
  double quantization_steps[3] = {0.0, 0.0, 0.0};
  double range_per_axis[3] = {0.0, 0.0, 0.0};
  for(int dim_idx = 0; dim_idx < 3; ++dim_idx) {

    range_per_axis[dim_idx] = (((double)node_extents.max_vertex()[dim_idx]) - ((double)node_extents.min_vertex()[dim_idx]));
    quantization_steps[dim_idx] = range_per_axis[dim_idx] / 65536.0;

    double normalized_position = (in_uncompressed_surfel.pos[dim_idx] - ((double)node_extents.min_vertex()[dim_idx]) )  / range_per_axis[dim_idx];

    int32_t overflow_protected_pos = 0;

    overflow_protected_pos = int32_t(std::round( ((in_uncompressed_surfel.pos[dim_idx] - ((double)node_extents.min_vertex()[dim_idx]) )  / quantization_steps[dim_idx] ) ) );

    out_compressed_surfel.pos_16ui_components[dim_idx] = std::min(int32_t(std::numeric_limits<uint16_t>::max()), std::max(int32_t(std::numeric_limits<uint16_t>::min()), overflow_protected_pos));
  }
}


void quantize_radius(surfel const& in_uncompressed_surfel, quantized_surfel& out_compressed_surfel, float const avg_surfel_radius, float const max_surfel_radius_deviation) {
  //currently, min and max radius are not exposed in the tree, so we should use the avg radius
  //to not introduce large errors, we need to calculate the range from avg rad to min and max rad and use the max range to quantize (this does not filter outliers)
  double one_sided_quantization_step = 0.0;

  one_sided_quantization_step = ((double)max_surfel_radius_deviation) / 127.0;

  if( 0.0 == one_sided_quantization_step ) { //avoid division by zero
    out_compressed_surfel.radius_scale = 127;
    return;
  }

  double reference_min_range = avg_surfel_radius - max_surfel_radius_deviation;
  double normalized_float_radius = ((double)in_uncompressed_surfel.size - reference_min_range) / (2*max_surfel_radius_deviation);

  int32_t overflow_protected_rad = std::round(normalized_float_radius * 254); //skip 1 out of 256 steps for convenience
  out_compressed_surfel.radius_scale = std::min( int32_t(254), std::max( int32_t(0), overflow_protected_rad) );
}

void quantize_color(surfel const& in_uncompressed_surfel, quantized_surfel& out_compressed_surfel) {
  int32_t rb_quantization_step = 8;
  int32_t g_quantization_step  = 4;

  int8_t r5 = int32_t( std::round(in_uncompressed_surfel.rgbf[0] / (double)rb_quantization_step) );
  int8_t g6 = int32_t( std::round(in_uncompressed_surfel.rgbf[1] /  (double)g_quantization_step) );
  int8_t b5 = int32_t( std::round(in_uncompressed_surfel.rgbf[2] / (double)rb_quantization_step) );

  r5 = std::max(int8_t(31), r5);
  g6 = std::max(int8_t(63), g6);
  b5 = std::max(int8_t(31), b5);

  uint8_t rb_combination_mask = 0x1F;
  uint8_t  g_combination_mask = 0x3F;


  //bit pattern: rrrrr gggggg bbbbb  <----- 16 bit
  uint16_t quantized_and_combined_color =  ((r5 & rb_combination_mask) << 11) |
                                           ((g6 &  g_combination_mask) <<  5) |
                                           ((b5 & rb_combination_mask) << 0);



  out_compressed_surfel.color_565ui_combined = quantized_and_combined_color;
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

  int32_t normal_positions_per_face = face_positions_u*face_positions_v;

  //face ids based on dominant axes: -x = 0; +x = 1; -y = 2; +y=3; -z = 4; +z = 5 
  int32_t dominant_face_idx = dominant_axis_idx*2 - ( in_uncompressed_surfel.normal[dominant_axis_idx] < 0.0 ? 1 : 0 );

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
    
    size_t depth = 0;
    if (depth > bvh->get_depth() || depth < 0) {
      depth = bvh->get_depth();
    }
    std::cout << "Starting to compress.. " << depth << std::endl;

    lamure::ren::lod_stream* in_access = new lamure::ren::lod_stream();
    in_access->open(input_uncompressed_lod_file_name);

    size_t size_of_node = (uint64_t)bvh->get_primitives_per_node() * sizeof(lamure::ren::dataset::serialized_surfel);
    surfel* surfels = new surfel[bvh->get_primitives_per_node()];

    lamure::node_t first_leaf = bvh->get_first_node_id_of_depth(depth);
    lamure::node_t num_leafs = bvh->get_length_of_depth(depth);

    std::ofstream out_stream;
    out_stream.open(out_lodqz_file, std::ios::out | std::ios::trunc);
    out_stream.close();

    //consider hidden translation
    //const scm::math::vec3f& translation = bvh->get_translation();

    uint64_t num_surfels_excluded = 0;
    

    auto const& bvh_bounding_boxes = bvh->get_bounding_boxes();

    //iterate over all nodes
    for (lamure::node_t node_idx = 0; node_idx < first_leaf + num_leafs; ++node_idx) {

      auto const& avg_surfel_radius = bvh->get_avg_primitive_extent(node_idx);
      auto const& max_radius_deviation = bvh->get_max_surfel_radius_deviation(node_idx);

#ifdef VERBOSE
        if ((leaf_id-first_leaf) % 1000 == 0) {
            std::cout << leaf_id-first_leaf << " / " << num_leafs << " writing: " << out_lodqz_file << std::endl;
        }
#endif
        
        in_access->read((char*)surfels, node_idx * size_of_node, size_of_node);

        std::ios::openmode mode = std::ios::out | std::ios::app;
        out_stream.open(out_lodqz_file, mode);
   
        std::string filestr;
        std::stringstream ss(filestr);
        

        double unquantized_pos[3];

        for (unsigned int i = 0; i < bvh->get_primitives_per_node(); ++i) {
            quantized_surfel qz_surfel;
            const surfel& s = surfels[i];

            //quantization
            quantize_complete_surfel(s, qz_surfel, bvh_bounding_boxes[node_idx], avg_surfel_radius, max_radius_deviation);


            //quantization error measurement check
            double squared_pos_error_comps = 0.0;
            for(int dim_idx = 0; dim_idx < 3; ++dim_idx) {
              unquantized_pos[dim_idx] = qz_surfel.pos_16ui_components[dim_idx] * ((((double)bvh_bounding_boxes[node_idx].max_vertex()[dim_idx]) - ((double)bvh_bounding_boxes[node_idx].min_vertex()[dim_idx])) / 65536.0 ) + ((double)bvh_bounding_boxes[node_idx].min_vertex()[dim_idx] );
              squared_pos_error_comps += (unquantized_pos[dim_idx] - s.pos[dim_idx])*(unquantized_pos[dim_idx] - s.pos[dim_idx]);
            }

            double pos_quant_error = std::sqrt(squared_pos_error_comps);
            

            std::cout << "Quantized surfel position: " << s.pos[0] << ", " << s.pos[1] << ", " << s.pos[2] << " to: " << unquantized_pos[0] << ", " << unquantized_pos[1] << ", " << unquantized_pos[2] << "\n";
            std::cout << "Pos Quantization Error: " << pos_quant_error << "\n";


            //quantization error measurement check
            double radius_quantization_error = 0.0;
            float unquantized_radius = qz_surfel.radius_scale * ((((double) ( max_radius_deviation + avg_surfel_radius ) ) - ((double) ( avg_surfel_radius - max_radius_deviation ) )) / 255.0 ) + ((double)avg_surfel_radius - max_radius_deviation);
            radius_quantization_error = std::abs(s.size - unquantized_radius);
            
            

            std::cout << "Quantized surfel radius: " << s.size << " to: " << unquantized_radius << "\n";
            std::cout << "Rad Quantization Error: " << radius_quantization_error << "\n";
            /*
            

            ss << s.pos[0] << " ";
            ss << s.pos[1] << " ";
            ss << s.pos[2] << " ";
             
            ss << s.normal[0] << " ";
            ss << s.normal[1] << " ";
            ss << s.normal[2] << " ";

            ss << (unsigned int)s.rgbf[0]<< " ";
            ss << (unsigned int)s.rgbf[1] << " ";
            ss << (unsigned int)s.rgbf[2] << " ";

            ss << s.size;
            

            ss << std::endl;
            */
        }


        out_stream << ss.rdbuf();
        out_stream.close();

    }

    std::cout << "done. (" << num_surfels_excluded << " surfels excluded)" << std::endl;

    delete[] surfels;
    delete in_access;
    delete bvh;


}
