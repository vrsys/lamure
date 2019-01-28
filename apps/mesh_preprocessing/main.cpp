// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
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
#include <set>

#include <lamure/types.h>
#include <lamure/ren/dataset.h>
#include <lamure/ren/bvh.h>
#include <lamure/ren/lod_stream.h>

struct rectangle {
  scm::math::vec2f min_;
  scm::math::vec2f max_;
  int id_;
  bool flipped_;
};

struct chart {
  int id_;
  rectangle rect_;
  lamure::bounding_box box_;
  std::set<int> all_triangle_ids_;
  std::set<int> original_triangle_ids_;
};


char* get_cmd_option(char** begin, char** end, const std::string & option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}


int load_chart_file(std::string chart_file, std::vector<int>& chart_id_per_triangle) {

  int num_charts = 0;

  std::ifstream file(chart_file);

  std::string line;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::string chart_id_str;

    while (std::getline(ss, chart_id_str, ' ')) {
      
      int chart_id = atoi(chart_id_str.c_str());
      num_charts = std::max(num_charts, chart_id+1);
      chart_id_per_triangle.push_back(chart_id);

      //std::cout << chart_id << std::endl;
    }
  }


  file.close();

  return num_charts;
}

int main(int argc, char *argv[]) {
    
    if (argc == 1 || 
      cmd_option_exists(argv, argv+argc, "-h") ||
      !cmd_option_exists(argv, argv+argc, "-f")) {
      
      std::cout << "Usage: " << argv[0] << "<flags> -f <input_file>" << std::endl <<
         "INFO: bvh_leaf_extractor " << std::endl <<
         "\t-f: selects .bvh input file" << std::endl <<
         "\t-c: selects .lodchart input file" << std::endl <<
         std::endl;
      return 0;
    }

    std::string bvh_filename = std::string(get_cmd_option(argv, argv + argc, "-f"));
    std::string chart_lod_filename = std::string(get_cmd_option(argv, argv + argc, "-c"));

    std::string lod_filename = bvh_filename.substr(0, bvh_filename.size()-3) + "lod";

    std::vector<int> chart_id_per_triangle;
    int num_charts = load_chart_file(chart_lod_filename, chart_id_per_triangle);
    std::cout << "lodchart file lodaded." << std::endl;

    std::shared_ptr<lamure::ren::bvh> bvh = std::make_shared<lamure::ren::bvh>(bvh_filename);
    std::cout << "bvh file loaded." << std::endl;

    std::shared_ptr<lamure::ren::lod_stream> lod = std::make_shared<lamure::ren::lod_stream>();
    lod->open(lod_filename);
    std::cout << "lod file loaded." << std::endl;

    
    std::vector<chart> charts;
    //initially, lets compile the charts

    for (int chart_id = 0; chart_id < num_charts; ++chart_id) {
      rectangle rect{
        scm::math::vec2f(std::numeric_limits<float>::max()),
        scm::math::vec2f(std::numeric_limits<float>::lowest()),
        chart_id, 
        false};
      lamure::bounding_box box(
        scm::math::vec3d(std::numeric_limits<float>::max()),
        scm::math::vec3d(std::numeric_limits<float>::lowest()));
      
      charts.push_back(chart{chart_id, rect, box, std::set<int>()});
    }


    //compute the bounding box
    size_t vertices_per_node = bvh->get_primitives_per_node();
    size_t size_of_vertex = bvh->get_size_of_primitive();
    
    std::vector<lamure::ren::dataset::serialized_vertex> vertices;
    vertices.resize(vertices_per_node);

    for (int node_id = 0; node_id < bvh->get_num_nodes(); node_id++) {
      
      lod->read((char*)&vertices[0], (vertices_per_node*node_id*size_of_vertex),
        (vertices_per_node * size_of_vertex));

      for (int vertex_id = 0; vertex_id < vertices_per_node; ++vertex_id) {
        int chart_id = chart_id_per_triangle[node_id*(vertices_per_node/3)+vertex_id/3];

        auto& chart = charts[chart_id];


        const auto& vertex = vertices[vertex_id];
        scm::math::vec3d v(vertex.v_x_, vertex.v_y_, vertex.v_z_);

        scm::math::vec3d min_vertex = chart.box_.min();
        scm::math::vec3d max_vertex = chart.box_.max();


        min_vertex.x = std::min(min_vertex.x, v.x);
        min_vertex.y = std::min(min_vertex.y, v.y);
        min_vertex.z = std::min(min_vertex.z, v.z);

        max_vertex.x = std::max(max_vertex.x, v.x);
        max_vertex.y = std::max(max_vertex.y, v.y);
        max_vertex.z = std::max(max_vertex.z, v.z);

        chart.box_ = lamure::bounding_box(min_vertex, max_vertex);

      }
    }

    for (int chart_id = 0; chart_id < num_charts; ++chart_id) {

      //intersect all bvh leaf nodes with chart bounding boxes
      auto& chart = charts[chart_id];

      uint32_t first_leaf = bvh->get_first_node_id_of_depth(bvh->get_depth()-1);
      uint32_t num_leafs = bvh->get_length_of_depth(bvh->get_depth()-1);

      for (int leaf_id = first_leaf; leaf_id < first_leaf+num_leafs; ++leaf_id) {
        lamure::bounding_box leaf_box(
          scm::math::vec3d(bvh->get_bounding_boxes()[leaf_id].min_vertex()),
          scm::math::vec3d(bvh->get_bounding_boxes()[leaf_id].max_vertex()));

        if (chart.box_.intersects(leaf_box)) {

          lod->read((char*)&vertices[0], (vertices_per_node*leaf_id*size_of_vertex),
            (vertices_per_node * size_of_vertex));

          for (int vertex_id = 0; vertex_id < vertices_per_node; ++vertex_id) {
            const auto& vertex = vertices[vertex_id];
            scm::math::vec3d v(vertex.v_x_, vertex.v_y_, vertex.v_z_);

            //find out if this triangle intersects the chart box
            if (chart.box_.contains(v)) {
              int lod_tri_id = leaf_id*(vertices_per_node/3)+(vertex_id/3);
              chart.all_triangle_ids_.insert(lod_tri_id);
            }
          }
        }

        for (int tri_id = 0; tri_id < vertices_per_node/3; ++tri_id) {
          int lod_tri_id = leaf_id*(vertices_per_node/3)+tri_id;
          if (chart_id == chart_id_per_triangle[lod_tri_id]) {
            chart.original_triangle_ids_.insert(lod_tri_id); 
          }
        }
      }
      
    }


    //TODO:
    //-determine the chart projection plane based on these original tris
    //-perform the projection for all triangle_ids_ 
    //-perform rectangle packing
    //-then texture rendering


    for (int chart_id = 0; chart_id < num_charts; ++chart_id) {
      auto& chart = charts[chart_id];
      std::cout << "chart id " << chart_id << std::endl;
      std::cout << "box min " << chart.box_.min() << std::endl;
      std::cout << "box max " << chart.box_.max() << std::endl;
      std::cout << "original triangles " << chart.original_triangle_ids_.size() << std::endl;
      std::cout << "all triangles " << chart.all_triangle_ids_.size() << std::endl;
    }

    std::cout << "tris per node " << vertices_per_node/3 << std::endl;


    bvh.reset();
    lod->close();
    lod.reset();

    return 0;
}



