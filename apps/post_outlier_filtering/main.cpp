// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
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


int main(int argc, char *argv[]) {

    if (argc < 3 ||
        cmd_option_exists(argv, argv+argc, "-h") ||
        (    !cmd_option_exists(argv, argv+argc, "-f") 
          || !cmd_option_exists(argv, argv+argc, "-o")
        ) ) {
        std::cout << "Usage: " << argv[0] << " -f <input_file>.bvh -o <output_file>.bvh -r <radius_factor>\n" <<
            "INFO: "<< argv[0] <<"\n" <<
            "\t-f: selects unfiltered .bvh input file\n" <<
            "\t    (-f flag is required)\n" <<
            "\t-o: writes filtered .bvh and corresponding .lod input file\n" <<
            "\t    (-o flag is required)\n" <<
            "\t-r: radius scaling factor compared to avg radius per level which classifies surfels als outlier.\n" <<
            "\t    (default: 2.0)\n" <<
            std::endl;
        return 0;
    }

    std::string const input_unfiltered_bvh_file_name = std::string(get_cmd_option(argv, argv + argc, "-f"));
    std::string input_base_name = input_unfiltered_bvh_file_name.substr(0, input_unfiltered_bvh_file_name.size()-3);
    std::string input_bvh_ext   = input_unfiltered_bvh_file_name.substr(input_unfiltered_bvh_file_name.size()-3);
    if (input_bvh_ext.compare("bvh") != 0) {
        std::cout << "please specify an (uncompressed).bvh file as input" << std::endl;
        return 0;
    }


    std::string const output_filtered_bvh_file_name = std::string(get_cmd_option(argv, argv + argc, "-o"));
    std::string output_base_name = output_filtered_bvh_file_name.substr(0, output_filtered_bvh_file_name.size()-3);
    std::string output_bvh_ext   = output_filtered_bvh_file_name.substr(output_filtered_bvh_file_name.size()-3);

    if(input_unfiltered_bvh_file_name == output_filtered_bvh_file_name) {
      std::cout << "ERROR: input name == output name. Aborting.\n";
      return -1;
    }

    double radius_deviation_to_avg_radius_per_level = 2.0;

    if(cmd_option_exists(argv, argv+argc, "-r")) {
      radius_deviation_to_avg_radius_per_level = std::atof(get_cmd_option(argv, argv + argc, "-r"));
    }


    std::cout << "Radius filtering factor: " << radius_deviation_to_avg_radius_per_level << "\n";

    std::string input_uncompressed_lod_file_name = input_base_name + "lod";

    std::cout << "Base filename: " << input_base_name << "\n";
    std::cout << "Extension: " << input_bvh_ext << "\n";


    std::string output_prefix = input_base_name;
    
    std::string out_filtered_bvh_file = output_base_name + "bvh";
    std::string out_filtered_lod_file = output_base_name + "lod";

       
    lamure::ren::bvh* bvh = new lamure::ren::bvh(input_unfiltered_bvh_file_name);
    
    size_t depth = -1;
    if (depth > bvh->get_depth() || depth < 0) {
      depth = bvh->get_depth();
    }
    std::cout << "Starting to Filter.. " << std::endl;

    lamure::ren::lod_stream* in_access = new lamure::ren::lod_stream();
    in_access->open(input_uncompressed_lod_file_name);

    size_t size_of_node = (uint64_t)bvh->get_primitives_per_node() * sizeof(lamure::ren::dataset::serialized_surfel);


    //std::vector<quantized_surfel> qz_surfels(bvh->get_primitives_per_node());

    lamure::node_t first_leaf = bvh->get_first_node_id_of_depth(depth);
    lamure::node_t num_leafs = bvh->get_length_of_depth(depth);

    std::vector<surfel>           surfels(bvh->get_primitives_per_node() * num_leafs);

    std::ofstream out_stream;
    out_stream.open(out_filtered_lod_file, std::ios::out | std::ios::trunc);
    out_stream.close();

    uint64_t num_surfels_excluded = 0;
    

    auto const& bvh_bounding_boxes = bvh->get_bounding_boxes();


    std::ios::openmode mode = std::ios::out | std::ios::binary;
    out_stream.open(out_filtered_lod_file, mode);

    int32_t num_levels_to_process = 0;
    int32_t nodes_for_current_level = num_leafs;
    while(nodes_for_current_level > 0) {
      ++num_levels_to_process;
      nodes_for_current_level /= 2;
    }


    std::size_t node_offset = 0;

    for(int32_t level_idx = 0; level_idx < num_levels_to_process; ++level_idx) {
      std::size_t nodes_in_current_level = 1 << level_idx;

      in_access->read((char*)&surfels[0], node_offset * size_of_node, nodes_in_current_level * size_of_node);

      int32_t const num_surfels_to_process_in_level = bvh->get_primitives_per_node() * nodes_in_current_level;
      
      double current_avg_surfel_radius = 0.0;
      #pragma omp parallel for reduction(+:current_avg_surfel_radius)
      for(int surfel_idx = 0; surfel_idx < num_surfels_to_process_in_level; ++surfel_idx) {
        current_avg_surfel_radius += surfels[surfel_idx].size;
      }

      current_avg_surfel_radius /= num_surfels_to_process_in_level;

      #pragma omp parallel for reduction(+:num_surfels_excluded)
      for(int surfel_idx = 0; surfel_idx < num_surfels_to_process_in_level; ++surfel_idx) {
        if(surfels[surfel_idx].size >  current_avg_surfel_radius * radius_deviation_to_avg_radius_per_level) {
          surfels[surfel_idx].size = 0.0;
          ++num_surfels_excluded;
        }
      }

      out_stream.write((char*) &surfels[0], nodes_in_current_level * bvh->get_primitives_per_node() * sizeof(surfel) );

      node_offset += nodes_in_current_level;
    }

/*    for (lamure::node_t node_idx = 0; node_idx < first_leaf + num_leafs; ++node_idx) {
      if(node_idx % 1000 == 0)
      std::cout << "Starting with: " << node_idx << "\r";
      std::cout.flush();
      auto const& avg_surfel_radius = bvh->get_avg_primitive_extent(node_idx);
      auto max_radius_deviation = bvh->get_max_surfel_radius_deviation(node_idx);

        
      in_access->read((char*)&surfels[0], node_idx * size_of_node, size_of_node);

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
          surfel& s = surfels[i];

          if(s.size > avg_surfel_radius * radius_deviation_to_avg_radius_per_level) {
            s.size = 0;
            ++num_surfels_excluded;
          }

      }
      out_stream.write((char*) &surfels[0], bvh->get_primitives_per_node() * sizeof(surfel) );
    }*/
    out_stream.close();

       


    bvh->set_size_of_primitive( sizeof(lamure::ren::dataset::serialized_surfel) );
    bvh->set_primitive(lamure::ren::bvh::primitive_type::POINTCLOUD);

    lamure::ren::bvh_stream bvh_ofstream;
    bvh_ofstream.write_bvh(out_filtered_bvh_file, *bvh);


    std::cout << "done. (" << num_surfels_excluded << " surfels excluded)" << std::endl;

    delete in_access;
    delete bvh;


}
