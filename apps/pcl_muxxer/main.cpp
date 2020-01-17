
#include "utils.h"
#include <lamure/mesh/tools.h>

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
      "\t-p0: select colored .xyz / .xyz_all input file\n" <<
      "\t-p1: select attribute .xyz / .xyz_all input file\n" <<
      "\t Info: Colored points from p0 will be augmented by attribs in p1\n" << 
      "\n";
    return 0;
  }

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

    //std::cout << "point_id " << point_id << std::endl;
    
#if 0
    pointclouds[0].prov_attribs_[i] = ((double)pointclouds[1].colors_[point_id][0])/(double)255.0;
#else
    if (pointclouds[1].colors_[point_id][0] > (uint8_t)216) {
      //  std::cout << "encountered " << pointclouds[1].colors_[point_id][0] << " to " << pointclouds[0].prov_attribs_[i] << std::endl;
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

  std::cout << "Done. Have a nice day." << std::endl;

  
  return 0;
}






