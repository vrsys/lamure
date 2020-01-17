
#ifndef UTILS_H_
#define UTILS_H_

#include <scm/core/math.h>

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



static char* get_cmd_option(char** begin, char** end, const std::string& option) {
  char** it = std::find(begin, end, option);
  if (it != end && ++it != end) {
    return *it;
  }
  return 0;
}

static bool cmd_option_exists(char** begin, char** end, const std::string& option) {
  return std::find(begin, end, option) != end;
}

struct pointcloud {
  std::vector<size_t> indices_;
  std::vector<Point> points_;
  std::vector<Vector> normals_;
  std::vector<Color> colors_;
  std::vector<float> radii_;
  std::vector<double> prov_attribs_;
};


static void read_xyz_file(pointcloud& pcl, const std::string& filename) {

  bool xyz_all = false;
  if (filename.substr(filename.size()-7).compare("xyz_all") == 0) {
    xyz_all = true;
  }
  else if (filename.substr(filename.size()-3).compare("xyz") == 0) {

  }
  else {
    std::cout << "Error: please specify a valid xyz or xyz_all file.\n";
    exit(1);
  }

  std::ifstream input_file(filename);

  if (!input_file.good()) {
    std::cout << "Error opening file: " << filename << "\n";
    exit(1);
  }

  std::string line_buffer;

  const int32_t precision = 15;

  while (getline(input_file, line_buffer)) {
    std::istringstream line(line_buffer);

    scm::math::vec3d pos(0.0);
    line >> std::setprecision(precision) >> pos.x;
    line >> std::setprecision(precision) >> pos.y;
    line >> std::setprecision(precision) >> pos.z;

    scm::math::vec3d nml(0.0);
    if (xyz_all) {
      line >> std::setprecision(precision) >> nml.x;
      line >> std::setprecision(precision) >> nml.y;
      line >> std::setprecision(precision) >> nml.z;
    }

    int32_t r, g, b;
    line >> r;
    line >> g;
    line >> b;

    double radius = 0.002;

    if (xyz_all) {
      line >> std::setprecision(precision) >> radius;
    }

    pcl.points_.push_back({pos.x, pos.y, pos.z});
    pcl.normals_.push_back({nml.x, nml.y, nml.z});
    pcl.colors_.push_back({(uint8_t)r, (uint8_t)g, (uint8_t)b});
    pcl.radii_.push_back(radius);
    
  }

  input_file.close();

  std::cout << pcl.points_.size() << " points loaded." << std::endl;
  
  pcl.indices_.resize(pcl.points_.size());

  for(std::size_t i = 0; i < pcl.points_.size(); ++i){
    pcl.indices_[i] = i;
  }


}

static void write_xyz_file(pointcloud& pcl, const std::string& filename) {
  
  //std::cout << "Write xyz file not implemented" << std::endl;

  std::string xyz_all_string = "";
  std::stringstream out_string(xyz_all_string);

  //clear file
  std::ofstream reset_file(filename, std::ios::out | std::ios::trunc);
  reset_file << out_string.rdbuf();
  reset_file.close();


  const int32_t precision = 15;

  for (uint64_t i = 0; i < pcl.points_.size(); ++i) {

    
    out_string << 
      std::setprecision(precision) << pcl.points_[i][0] << " " << 
      std::setprecision(precision) << pcl.points_[i][1] << " " << 
      std::setprecision(precision) << pcl.points_[i][2] << " " <<
      std::setprecision(precision) << pcl.normals_[i][0] << " " << 
      std::setprecision(precision) << pcl.normals_[i][1] << " " << 
      std::setprecision(precision) << pcl.normals_[i][2] << " " <<
      (int)pcl.colors_[i][0] << " " << 
      (int)pcl.colors_[i][1] << " " << 
      (int)pcl.colors_[i][2] << " " <<
      std::setprecision(precision) << pcl.radii_[i] << "\n";

      if ((i % (uint64_t)10000) == 0) {
        std::ofstream out_file(filename, std::ios::out | std::ios::app);
        out_file << out_string.rdbuf();   
        out_file.close();
        out_string.str(std::string());
        out_string.clear();
      }

  }

  //write teh rest
  std::ofstream out_file(filename, std::ios::out | std::ios::app);
  out_file << out_string.rdbuf(); 
  out_file.close();


  

}



#endif
