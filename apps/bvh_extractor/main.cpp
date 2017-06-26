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
#include <algorithm>
#include <lamure/ren/model_database.h>
#include <lamure/bounding_box.h>

#include <lamure/types.h>
#include <lamure/ren/dataset.h>
#include <lamure/ren/bvh.h>
#include <lamure/ren/lod_stream.h>

#define VERBOSE
#define DEFAULT_PRECISION 15

char* get_cmd_option(char** begin, char** end, const std::string & option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

struct xyzall_surfel_t {
  float x_, y_, z_;
  uint8_t r_, g_, b_, fake_;
  float size_;
  float nx_, ny_, nz_;
};

enum type_t {
  TYPE_INVALID = 0,
  TYPE_XYZ = 1,
  TYPE_XYZ_ALL = 2,
};

int main(int argc, char *argv[]) {
    
    if (argc == 1 || 
      cmd_option_exists(argv, argv+argc, "-h") ||
      !cmd_option_exists(argv, argv+argc, "-f")) {
        
      std::cout << "Usage: " << argv[0] << "<flags> -f <input_file>" << std::endl <<
         "INFO: bvh_leaf_extractor " << std::endl <<
         "\t-f: selects .bvh input file" << std::endl <<
         "\t    (-f flag is required) " << std::endl <<
         "\t-m: select output file extension" << std::endl <<
         "\t    (options: \"xyz\", \"xyz_all\")" << std::endl <<
         "\t    (default: \"xyz_all\")" << std::endl <<
         "\t-d: select depth to extract (optional)" << std::endl <<
         std::endl;
      return 0;
    }

    std::string bvh_filename = std::string(get_cmd_option(argv, argv + argc, "-f"));

    std::string ext = bvh_filename.substr(bvh_filename.size()-3);
    if (ext.compare("bvh") != 0) {
        std::cout << "please specify a .bvh file as input" << std::endl;
        return 0;
    }

    type_t type = TYPE_XYZ_ALL;
    if (cmd_option_exists(argv, argv+argc, "-m")) {
       std::string mode = get_cmd_option(argv, argv+argc, "-m");
       if (mode.compare("xyz") == 0) {
          type = TYPE_XYZ;
       }    
    }

    std::string xyz_filename = bvh_filename.substr(0, bvh_filename.size()-3) + "xyz_all";
    if (type == TYPE_XYZ) {
       xyz_filename = bvh_filename.substr(0, bvh_filename.size()-3) + "xyz";
    }

    if (cmd_option_exists(argv, argv+argc, "-o")) {
       xyz_filename = std::string(get_cmd_option(argv, argv + argc, "-o"));
       if (type == TYPE_XYZ_ALL) {
         if ((xyz_filename.substr(xyz_filename.size()-7)).compare("xyz_all") != 0) {
            std::cout << "inconsistent output file extension encountered" << std::endl;
            std::cout << "terminating..." << std::endl;
            return 0;
         }
       }
       else {
         if ((xyz_filename.substr(xyz_filename.size()-3)).compare("xyz") != 0) {
            std::cout << "inconsistent output file extension encountered" << std::endl;
            std::cout << "terminating..." << std::endl;
            return 0;
         }
       }

    }
    
    int32_t depth = -1;
    if (cmd_option_exists(argv, argv+argc, "-d")) {
       depth = atoi(get_cmd_option(argv, argv+argc, "-d"));
    }

    std::cout << "input: " << bvh_filename << std::endl;
    std::cout << "output: " << xyz_filename << std::endl;
   

    lamure::ren::bvh* bvh = new lamure::ren::bvh(bvh_filename);
    
    if (depth > bvh->get_depth() || depth < 0) {
      depth = bvh->get_depth();
    }
    std::cout << "extracting depth " << depth << std::endl;

    std::string lod_filename = bvh_filename.substr(0, bvh_filename.size()-3) + "lod";
    lamure::ren::lod_stream* in_access = new lamure::ren::lod_stream();
    in_access->open(lod_filename);

    size_t size_of_node = (uint64_t)bvh->get_primitives_per_node() * sizeof(lamure::ren::dataset::serialized_surfel);
    xyzall_surfel_t* surfels = new xyzall_surfel_t[bvh->get_primitives_per_node()];

    lamure::node_t first_leaf = bvh->get_first_node_id_of_depth(depth);
    lamure::node_t num_leafs = bvh->get_length_of_depth(depth);

    std::ofstream out_stream;
    out_stream.open(xyz_filename, std::ios::out | std::ios::trunc);
    out_stream.close();

    //consider hidden translation
    const scm::math::vec3f& translation = bvh->get_translation();

    uint64_t num_surfels_excluded = 0;
    
    for (lamure::node_t leaf_id = first_leaf; leaf_id < first_leaf + num_leafs; ++leaf_id) {

#ifdef VERBOSE
        if ((leaf_id-first_leaf) % 1000 == 0) {
            std::cout << leaf_id-first_leaf << " / " << num_leafs << " writing: " << xyz_filename << std::endl;
        }
#endif
        
        in_access->read((char*)surfels, leaf_id * size_of_node, size_of_node);

        std::ios::openmode mode = std::ios::out | std::ios::app;
        out_stream.open(xyz_filename, mode);
   
        std::string filestr;
        std::stringstream ss(filestr);
        
        for (unsigned int i = 0; i < bvh->get_primitives_per_node(); ++i) {
            const xyzall_surfel_t& s = surfels[i];

            if (s.size_ <= 0.0f) {
              ++num_surfels_excluded;
              continue;
            }
            

            ss << std::setprecision(DEFAULT_PRECISION) << translation.x + s.x_ << " ";
            ss << std::setprecision(DEFAULT_PRECISION) << translation.y + s.y_ << " ";
            ss << std::setprecision(DEFAULT_PRECISION) << translation.z + s.z_ << " ";
             
            if (type == TYPE_XYZ_ALL) {
              ss << std::setprecision(DEFAULT_PRECISION) << s.nx_ << " ";
              ss << std::setprecision(DEFAULT_PRECISION) << s.ny_ << " ";
              ss << std::setprecision(DEFAULT_PRECISION) << s.nz_ << " ";
            }

            ss << (unsigned int)s.r_ << " ";
            ss << (unsigned int)s.g_ << " ";
            ss << (unsigned int)s.b_ << " ";

            if (type == TYPE_XYZ_ALL) {
              ss << std::setprecision(DEFAULT_PRECISION) << s.size_;
            }
            
            ss << std::endl;
        }


        out_stream << std::setprecision(DEFAULT_PRECISION) << ss.rdbuf();
        out_stream.close();

    }

    std::cout << "done. (" << num_surfels_excluded << " surfels excluded)" << std::endl;

    delete[] surfels;
    delete in_access;
    delete bvh;


    return 0;
}



