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
    float x, y, z;
    uint8_t r, g, b, fake;
    float size;
    float nx, ny, nz;
};

int main(int argc, char *argv[]) {

    if (argc == 1 ||
        cmd_option_exists(argv, argv+argc, "-h") ||
        !cmd_option_exists(argv, argv+argc, "-o") ||
        !cmd_option_exists(argv, argv+argc, "-f")) {
        std::cout << "Usage: " << argv[0] << " <flags> -f <input_file>\n" <<
            "INFO: lod_converter\n" <<
            "\t-f: selects (knobi).lod input file\n" <<
            "\t    (-f flag is required)\n" <<
            "\t-o: selects outut prefix (without extensions)\n" << 
            "\t    (-o flag is required)\n" << 
            std::endl;
        return 0;
    }

    std::string input_knobi_file = std::string(get_cmd_option(argv, argv + argc, "-f"));

    std::string input_knobi_ext = input_knobi_file.substr(input_knobi_file.size()-3);
    if (input_knobi_ext.compare("lod") != 0) {
        std::cout << "please specify a (knobi).lod file as input" << std::endl;
        return 0;
    }

    std::string output_prefix = std::string(get_cmd_option(argv, argv+argc, "-o"));
    
    std::string out_lod_file = output_prefix + ".lod";
    std::string out_bvh_file = output_prefix + ".bvh";   
 
    std::cout << "converting entire lod hierarchy" << std::endl
              << "from: " << input_knobi_file << std::endl
              << "to: " << out_lod_file << std::endl;
       
    //prepare lod output file
    lamure::ren::LodStream* out_lod_access = new lamure::ren::LodStream();
    out_lod_access->openForWriting(out_lod_file);

    file_handler knobi_file_handler;
    
    knobi_file_handler.load(input_knobi_file.substr(0, input_knobi_file.size()-4));

    unsigned int num_nodes = knobi_file_handler.NodesInTree();
    unsigned int fan_factor = 2;
    unsigned int depth = knobi_file_handler.TreeDepth(); 
    depth = depth == 0 ? 0 : depth-1;
    unsigned int num_leafes = knobi_file_handler.LeafesCount();
    unsigned int num_surfels_per_node = knobi_file_handler.Clustersize();
    unsigned int num_points = num_nodes * num_surfels_per_node;

    repositoryPoints* knobi_lodRep = knobi_file_handler.PointsRepository();
    serializedTree* knobi_serTree = knobi_file_handler.SerialTree();
    
    surfel* data = new surfel[num_surfels_per_node];
    size_t size_of_node_in_bytes = sizeof(surfel) * num_surfels_per_node;
    memset((char*)data, 0, size_of_node_in_bytes);

    std::vector<float> average_surfels_per_node;
    average_surfels_per_node.resize(num_nodes);

    lamure::ren::bvh* bvh = new lamure::ren::bvh();
    bvh->set_num_nodes(num_nodes);
    bvh->set_fan_factor(fan_factor);
    bvh->set_depth(depth);
    bvh->set_surfels_per_node(num_surfels_per_node);
    bvh->set_size_of_surfel(sizeof(surfel));
    bvh->set_translation(scm::math::vec3f(0.f, 0.f, 0.f));

    for (unsigned int node_id = 0; node_id < num_nodes; ++node_id) {

#if 1
      unsigned int progress = (unsigned int )(((float)(node_id) / (float)num_nodes) * 25);
      std::cout << "\rLOG: [";
      for (unsigned int p = 0; p < 25; ++p) {
         if (p < progress)
             std::cout << "#";
         else
             std::cout << ".";
      }
      std::cout << "] progress..." << std::flush;

#endif

      serializedTree* knobi_node = knobi_serTree + node_id;       
      float quant_factor = knobi_node->QuantFactor;
      float min_surfel_size = knobi_node->minsurfelsize;
      float representative_surfel_size = 0.f;
   
      for (unsigned int surfel_id = 0; surfel_id < num_surfels_per_node; ++surfel_id) {
        repositoryPoints* point = (knobi_lodRep + knobi_node->repoidx + surfel_id);

        surfel s;
        s.x = point->x;
        s.y = point->y;
        s.z = point->z;
        s.r = (uint8_t)point->r;
        s.g = (uint8_t)point->g;
        s.b = (uint8_t)point->b;
        s.nx = point->nx;
        s.ny = point->ny;
        s.nz = point->nz;
        s.size = 0.5f * (((point->size / 255.f) * quant_factor) + min_surfel_size);
        data[surfel_id] = s;

        representative_surfel_size += s.size;
      
      }
      
      //store representative
      representative_surfel_size = representative_surfel_size / (float)num_surfels_per_node;
      out_lod_access->write((char*)data, node_id * size_of_node_in_bytes, size_of_node_in_bytes);
   
      scm::math::vec3f bb_min(scm::math::vec3f(knobi_node->boundbox.minx, knobi_node->boundbox.miny, knobi_node->boundbox.minz));
      scm::math::vec3f bb_max(scm::math::vec3f(knobi_node->boundbox.maxx, knobi_node->boundbox.maxy, knobi_node->boundbox.maxz));
      scm::math::vec3f bb_centroid = 0.5f * (bb_min + bb_max);

      bvh->Setbounding_box(node_id, scm::gl::boxf(bb_min, bb_max));
      bvh->SetCentroid(node_id, bb_centroid);
      bvh->SetAvgsurfelRadius(node_id, representative_surfel_size);
      bvh->SetVisibility(node_id, lamure::ren::bvh::node_visibility::NODE_VISIBLE);
       
    }

    bvh->writebvhfile(out_bvh_file);

    delete bvh;
    delete[] data;
    delete out_lod_access;

}
