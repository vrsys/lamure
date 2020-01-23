
#include "utils.h"

#include <lamure/mesh/tools.h>
#include <lamure/ren/bvh.h>
#include <lamure/ren/lod_stream.h>
#include <lamure/types.h>
#include <lamure/ren/dataset.h>



void leaf_level_algorithm(
  const std::string& pcl_t0_filename,
  const std::string& pcl_t1_filename) {

  std::cout << "Loading pointclouds..." << std::endl;

  std::vector<pointcloud> pointclouds(2);

  //load pointclouds
  read_xyz_file(pointclouds[0], pcl_t0_filename);
  read_xyz_file(pointclouds[1], pcl_t1_filename);

  std::cout << "Computing distances..." << std::endl;

  //compute nearest neighbour
  for (auto& pcl : pointclouds) {
    pcl.prov_attribs_.resize(pcl.points_.size(), 0.0);
  }

  uint32_t pcl_tree_idx = 1;
  Tree search_tree(
    boost::make_zip_iterator(boost::make_tuple(pointclouds[pcl_tree_idx].points_.begin(), pointclouds[pcl_tree_idx].indices_.begin())),
    boost::make_zip_iterator(boost::make_tuple(pointclouds[pcl_tree_idx].points_.end(), pointclouds[pcl_tree_idx].indices_.end()))  
  );


#if 0
  uint64_t num_points = pointclouds[0].points_.size();

  auto lambda_compute_attribs = [&](uint64_t i, uint32_t id)->void{
    lamure::mesh::show_progress(i, num_points);

    Point query(pointclouds[0].points_[i]);
    Neighbor_search search(search_tree, query, 1);
    uint64_t point_id = search.begin()->first.get<1>();
    
    pointclouds[0].prov_attribs_[i] = ((double)pointclouds[1].colors_[point_id][0])/(double)255.0;

  };

  uint32_t num_threads = (uint32_t)std::min(num_points, (uint64_t)8);

  lamure::mesh::parallel_for(num_threads, num_points, lambda_compute_attribs);

#else //single threaded

  for (uint64_t i = 0; i < pointclouds[0].points_.size(); ++i) {
    Point query(pointclouds[0].points_[i]);
    Neighbor_search search(search_tree, query, 1);
    uint64_t point_id = search.begin()->first.get<1>();
    
#if 0
    pointclouds[0].prov_attribs_[i] = ((double)pointclouds[1].colors_[point_id][0])/(double)255.0;
#else
    if (pointclouds[1].colors_[point_id][0] > (uint8_t)216) {
      pointclouds[0].colors_[i][0] = (uint8_t)pointclouds[1].colors_[point_id][0];
      pointclouds[0].colors_[i][1] = (uint8_t)25;
      pointclouds[0].colors_[i][2] = (uint8_t)25;
    }
#endif
    
  }
#endif

#if 1
  std::string pcl_result_file = pcl_t0_filename.substr(0, pcl_t0_filename.size()-8) + "_red_cracks.xyz_all";

  std::cout << "Write pointcloud with cracks colored in red " << pcl_result_file << std::endl;

  write_xyz_file(pointclouds[0], pcl_result_file);

  

#else
  std::string pcl_result_file = pcl_t0_filename.substr(0, pcl_t0_filename.size()-8) + "_prov_attribs.prov";

  std::cout << "Write provenance attributes to file " << pcl_result_file << std::endl;

  std::ofstream out_file(pcl_result_file, std::ios::out);

  for (const auto attrib : pointclouds[0].prov_attribs_) {
    out_file << attrib << "\n";
  }

  out_file.close();
#endif

}


void lod_algorithm(const std::string& pcl_t0_bvh_filename, const std::string& pcl_t1_xyz_filename) {


  struct xyzall_surfel_t {
    float x_, y_, z_;
    uint8_t r_, g_, b_, fake_;
    float size_;
    float nx_, ny_, nz_;
  };

  std::cout << "Loading xyz_all file..." << std::endl;

  pointcloud pcl_t1;
  read_xyz_file(pcl_t1, pcl_t1_xyz_filename);
  pcl_t1.prov_attribs_.resize(pcl_t1.points_.size(), 0.0);

  std::cout << "Building search tree..." << std::endl;

  Tree search_tree(
    boost::make_zip_iterator(boost::make_tuple(pcl_t1.points_.begin(), pcl_t1.indices_.begin())),
    boost::make_zip_iterator(boost::make_tuple(pcl_t1.points_.end(), pcl_t1.indices_.end()))  
  );


  std::cout << "Loading .bvh / .lod file..." << std::endl;

  std::string pcl_t0_lod_filename = pcl_t0_bvh_filename.substr(0, pcl_t0_bvh_filename.size()-3) + "lod";


  lamure::ren::bvh* bvh = new lamure::ren::bvh(pcl_t0_bvh_filename);
  
  lamure::ren::lod_stream* lod = new lamure::ren::lod_stream();
  lod->open(pcl_t0_lod_filename);


  size_t size_of_node = (uint64_t)bvh->get_primitives_per_node() * sizeof(lamure::ren::dataset::serialized_surfel);
  
  xyzall_surfel_t* surfels = new xyzall_surfel_t[bvh->get_primitives_per_node()];

  std::cout << "Generating prov attribs..." << std::endl;

  std::vector<float> prov_attribs_;

  for (int32_t depth = 0; depth <= bvh->get_depth(); ++depth) {
    lamure::node_t first_node = bvh->get_first_node_id_of_depth(depth);
    lamure::node_t num_nodes = bvh->get_length_of_depth(depth);  

    for (lamure::node_t node_id = first_node; node_id < first_node+num_nodes; ++node_id) {

      //load surfels for node
      lod->read((char*)surfels, node_id * size_of_node, size_of_node);

      //convert
      for (uint32_t i = 0; i < bvh->get_primitives_per_node(); ++i) {
        xyzall_surfel_t& s = surfels[i];
        
  
        Point query(s.x_, s.y_, s.z_);

        int32_t num_neighbours = ((int32_t)bvh->get_depth()+1)-(int32_t)depth;

        Neighbor_search search(search_tree, query, num_neighbours);

        double weight_total = 0.0;
        double prov_attrib = 0.0;

        for (auto it = search.begin(); it != search.end(); ++it) {
          uint64_t point_id = it->first.get<1>();
          
          if ((int32_t)pcl_t1.colors_[point_id][0] >= (int32_t)216) {
            weight_total += 1.0;

            prov_attrib += (double)pcl_t1.colors_[point_id][0];
          }
        }

        if (weight_total > 0.0) {
          double result = (prov_attrib / weight_total)/255.0;
          prov_attribs_.push_back(result);
        }
        else {
          prov_attribs_.push_back(0.0);
        }

      }
 
    }
  }

  std::cout << "Writing .prov file..." << std::endl;

  std::string out_prov_filename = pcl_t0_bvh_filename.substr(0, pcl_t0_bvh_filename.size()-3) + "prov";
  std::ofstream out_prov_file;
  out_prov_file.open(out_prov_filename.c_str(), std::ios::trunc | std::ios::binary);

  out_prov_file.write((char*)&prov_attribs_[0], prov_attribs_.size()*sizeof(float));


  out_prov_file.close();

  delete bvh;
  lod->close();
  delete lod;

  



}


int32_t main(int argc, char* argv[]) {

  std::string pcl_t0_filename = "";
  std::string pcl_t1_filename = "";
  if (cmd_option_exists(argv, argv+argc, "-p0")) {
    pcl_t0_filename = get_cmd_option(argv, argv+argc, "-p0");
  }
  if (cmd_option_exists(argv, argv+argc, "-p1")) {
    pcl_t1_filename = get_cmd_option(argv, argv+argc, "-p1");
  }

  if (!cmd_option_exists(argv, argv+argc, "-p0")
    || !cmd_option_exists(argv, argv+argc, "-p1")
    ) {
    std::cout << "Usage: " << argv[0] << "<flags>\n" <<
      "INFO: " << argv[0] << "\n" <<
      "\t-p0: select .bvh / .xyz / .xyz_all input file\n" <<
      "\t-p1: select attribute .xyz / .xyz_all input file\n" <<
      "\t Info: Colored points from p0 will be augmented by attribs in p1\n" << 
      "\n";
    return 0;
  }

  if (pcl_t0_filename.substr(pcl_t0_filename.find_last_of(".") + 1).compare("bvh") == 0) {
    std::cout << "Input file .bvh -> LOD Provenance Algorithm" << std::endl;
    lod_algorithm(pcl_t0_filename, pcl_t1_filename);
  }
  else if (pcl_t0_filename.substr(pcl_t0_filename.find_last_of(".") + 1).compare("xyz") == 0) {
    std::cout << "Input file .xyz -> Leaf-level Algorithm" << std::endl;
    leaf_level_algorithm(pcl_t0_filename, pcl_t1_filename);
  }
  else if (pcl_t0_filename.substr(pcl_t0_filename.find_last_of(".") + 1).compare("xyz_all") == 0) {
    std::cout << "Input file .xyz_all -> Leaf-level Algorithm" << std::endl;
    leaf_level_algorithm(pcl_t0_filename, pcl_t1_filename);
  }


  std::cout << "Done. Have a nice day." << std::endl;

  
  return 0;
}






