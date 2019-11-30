
#ifndef CGAL_H_INCLUDED_
#define CGAL_H_INCLUDED_



#include <iostream>
#include <string>
#include <functional>
#include <utility>
#include <fstream>

#include <memory>


#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/property_map.h>
#include <CGAL/IO/read_ply_points.h>
#include <utility>
#include <vector>
#include <fstream>

#include <CGAL/jet_estimate_normals.h>
#include <CGAL/Point_with_normal_3.h>
#include <CGAL/Search_traits_adapter.h>
#include <CGAL/Orthogonal_k_neighbor_search.h>
#include <CGAL/Search_traits_2.h>
#include <CGAL/property_map.h>
#include <boost/iterator/zip_iterator.hpp>
#include <CGAL/compute_average_spacing.h>


#include <lamure/types.h>
#include <scm/core/math.h>

#include <lamure/pre/surfel.h>
#include <lamure/pre/octree.h>
#include <lamure/pre/plane.h>
#include <lamure/mesh/tools.h>

#define DEFAULT_PRECISION 15


typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::Point_3 Point;
typedef Kernel::Vector_3 Vector;
typedef CGAL::cpp11::array<unsigned char, 3> Color;

typedef boost::tuple<Point, int> Point_and_int;
typedef CGAL::Search_traits_3<Kernel> Traits_base;
typedef CGAL::Search_traits_adapter<Point_and_int,
  CGAL::Nth_of_tuple_property_map<0, Point_and_int>,
  Traits_base> TreeTraits;

typedef CGAL::Orthogonal_k_neighbor_search<TreeTraits> Neighbor_search;
typedef Neighbor_search::Tree Tree;


struct pointcloud_t {
  std::vector<size_t> indices_;
  std::vector<Point> points_;
  std::vector<Vector> normals_;
  std::vector<Color> colors_;
  std::vector<float> radii_;
  //std::vector<double> distances_;
};



void nearest_neighbours(const scm::math::vec3d& p, 
  std::vector<scm::math::vec3d>& result, uint64_t num_neighbours,
  Tree& search_tree, pointcloud_t& pcl) {

  if (num_neighbours > pcl.points_.size()) {
    num_neighbours = pcl.points_.size();
  }
  if (num_neighbours == 0) {
    return;
  }

  result.clear();

  Point query(p.x, p.y, p.z);
  Neighbor_search::Distance distance;
  Neighbor_search search(search_tree, query, num_neighbours);

  //distances_[i] = std::abs(distance.inverse_of_transformed_distance(search.begin()->second));

  for (const auto it : search) {
    result.push_back(scm::math::vec3d(it.first.get<0>().x(), it.first.get<0>().y(), it.first.get<0>().z()));
  }


}



void compute_normals(std::vector<lamure::pre::surfel>& pointcloud) {

  std::cout << "Constructing search tree..." << std::endl;

  pointcloud_t pcl;
  for (uint64_t i = 0; i < pointcloud.size(); ++i) {
    const auto& xyz = pointcloud[i];
    pcl.indices_.push_back(i);
    pcl.points_.push_back({xyz.pos().x, xyz.pos().y, xyz.pos().z});
    pcl.normals_.push_back({xyz.normal().x, xyz.normal().y, xyz.normal().z});
    pcl.colors_.push_back({xyz.color().x, xyz.color().y, xyz.color().z});
    pcl.radii_.push_back(xyz.radius());
    //pcl.distances_.push_back(0.f);
  }

  Tree search_tree(
    boost::make_zip_iterator(boost::make_tuple(pcl.points_.begin(), pcl.indices_.begin())),
    boost::make_zip_iterator(boost::make_tuple(pcl.points_.end(), pcl.indices_.end()))  
  );


  uint32_t num_neighbours = 16;
  uint32_t num_threads = 32;

  uint64_t num_points = pointcloud.size();


  auto lambda_compute_normals = [&](uint64_t i, uint32_t id)->void{
    lamure::mesh::show_progress(i, num_points);

    auto& surfel = pointcloud[i];

    std::vector<uint64_t> neighbour_ids;
    scm::math::vec3d query(surfel.pos().x, surfel.pos().y, surfel.pos().z);

    std::vector<scm::math::vec3d> neighbours;
    nearest_neighbours(query, neighbours, num_neighbours, search_tree, pcl);

    lamure::pre::plane_t plane;
    lamure::pre::plane_t::fit_plane(neighbours, plane);

    scm::math::vec3d normal = scm::math::normalize(plane.get_normal());

    surfel.normal().x = normal.x;
    surfel.normal().y = normal.y;
    surfel.normal().z = normal.z;

  };


  lamure::mesh::parallel_for(num_threads, num_points, lambda_compute_normals);

  pcl.indices_.clear();
  pcl.points_.clear();
  pcl.normals_.clear();
  pcl.colors_.clear();
  pcl.radii_.clear();

}

void load_pointcloud(const std::string xyz_filename, std::vector<lamure::pre::surfel>& pointcloud, std::vector<int32_t>& scanner_ids) {
 
  bool xyz_all = (xyz_filename.substr(xyz_filename.size()-8) == ".xyz_all");
  if (!xyz_all) {
    if (xyz_filename.substr(xyz_filename.size()-4) != ".xyz") {
      std::cout << "ERROR: Invalid input format. Expected .xyz or .xyz_all" << std::endl;
      std::exit(1);
    }
  }

  std::ifstream input_file(xyz_filename.c_str());

  std::string line;
  while(getline(input_file, line)) {

    std::istringstream lineparser(line);

    scm::math::vec3d pos;
    lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.x;
    lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.y;
    lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.z;


    scm::math::vec3f normal(0.0);
    if (xyz_all) {
      lineparser >> std::setprecision(DEFAULT_PRECISION) >> normal.x;
      lineparser >> std::setprecision(DEFAULT_PRECISION) >> normal.y;
      lineparser >> std::setprecision(DEFAULT_PRECISION) >> normal.z;
    }

    scm::math::vec3d color;
    lineparser >> color.x;
    lineparser >> color.y;
    lineparser >> color.z;

    lamure::vec3b bcolor(color.x, color.y, color.z);
    
    double radius = 1.0;
    //if (xyz_all) {
      lineparser >> std::setprecision(DEFAULT_PRECISION) >> radius;
    //}

    scanner_ids.push_back((int32_t)radius);

    lamure::pre::surfel surfel(pos, bcolor, 0.01, normal, 0.0);
    pointcloud.push_back(surfel);



  }

  input_file.close();


}


void get_string(const lamure::pre::surfel& surfel, std::string& line, bool xyz_all) {

  line += std::to_string(surfel.pos().x) + " ";
  line += std::to_string(surfel.pos().y) + " ";
  line += std::to_string(surfel.pos().z) + " ";

  if (xyz_all) {
    line += std::to_string(surfel.normal().x) + " ";
    line += std::to_string(surfel.normal().y) + " ";
    line += std::to_string(surfel.normal().z) + " ";
  }

  line += std::to_string((int)surfel.color().x) + " ";
  line += std::to_string((int)surfel.color().y) + " ";
  line += std::to_string((int)surfel.color().z) + " ";

  if (xyz_all) {
    line += std::to_string(surfel.radius()) + " ";
  }

  line += "\n";

}


void write_pointcloud(
  const std::string& xyz_filename, const std::vector<lamure::pre::surfel>& pointcloud) {

  bool xyz_all = true;

  std::ofstream output_file(xyz_filename.c_str(), std::ios::app);

  std::string line = "";
  for (const auto& surfel : pointcloud) {
    get_string(surfel, line, xyz_all);
    output_file << line;
    line = "";
  }

  output_file.close();   

}

void face_normals_to_scanner(
  const std::vector<scm::math::vec3d>& scanner_positions, std::vector<lamure::pre::surfel>& pointcloud, std::vector<int32_t>& scanner_ids) {

  for (uint64_t point_id = 0; point_id < pointcloud.size(); ++point_id) {

    auto& surfel = pointcloud[point_id];
    auto& scanner_pos = scanner_positions[scanner_ids[point_id]];

    scm::math::vec3d normal(surfel.normal().x, surfel.normal().y, surfel.normal().z);

    double result = scm::math::dot(scm::math::normalize(normal), scm::math::normalize(scanner_pos - surfel.pos()));
    if (result <= 0.0) {
      surfel.normal() *= -1.0;
    }
  }

}



#endif
