
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


void nearest_neighbours(
  const std::vector<lamure::pre::surfel>& surfels, scm::math::vec3d& p, 
  std::vector<uint64_t>& result, uint64_t num_neighbours) {

  if (num_neighbours > surfels.size()) {
    num_neighbours = surfels.size();
  }
  if (num_neighbours == 0) {
    return;
  }

  result.clear();

  pointcloud_t pcl;
  for (uint64_t i = 0; i < surfels.size(); ++i) {
    const auto& xyz = surfels[i];
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

  Point query(p.x, p.y, p.z);
  Neighbor_search::Distance distance;
  Neighbor_search search(search_tree, query, num_neighbours);

  //distances_[i] = std::abs(distance.inverse_of_transformed_distance(search.begin()->second));

  for (const auto it : search) {
    for (uint64_t i = 0; i < pcl.points_.size(); ++i) {
      if (pcl.points_[i] == it.first.get<0>()) {
        result.push_back(i);
        i = pcl.points_.size();
      }
    }
  }


}



#endif
