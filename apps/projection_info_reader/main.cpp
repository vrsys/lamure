
#include <iostream>
#include <memory>
#include <vector>

#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>

#include <scm/core/math.h>




struct rectangle{
	scm::math::vec2f min_;
	scm::math::vec2f max_;
	int id_;
  bool flipped_;
};

struct projection_info {
  scm::math::vec3f proj_centroid;
  scm::math::vec3f proj_normal;
  scm::math::vec3f proj_world_up;

  rectangle tex_space_rect;
  scm::math::vec2f tex_coord_offset;
};




//entry point
int main(int argc, char **argv) {

  std::string infile = "data/chart.chartproj";

  //open file for reading
  std::ifstream input_file(infile, std::ios::binary);

  //get number of charts
  uint32_t num_charts;
  input_file.read((char*) &num_charts, sizeof(num_charts));

  std::cout << "Reading " << num_charts << " charts from file: " << infile << std::endl;

  projection_info chart_projections[num_charts];

  input_file.read((char*)&chart_projections, sizeof(chart_projections));
  input_file.close();

//testing
  // for (uint32_t i = 0; i < num_charts; ++i)
  // {
  //   projection_info proj = chart_projections[i];

  //   std::cout << "Proj centroid = [" << proj.proj_centroid.x << ", " << proj.proj_centroid.y << ", " << proj.proj_centroid.z << "]\n";
  //   std::cout << "Proj normal = [" << proj.proj_normal.x << ", " << proj.proj_normal.y << ", " << proj.proj_normal.z << "]\n";
  //   std::cout << "Proj world up = [" << proj.proj_world_up.x << ", " << proj.proj_world_up.y << ", " << proj.proj_world_up.z << "]\n"; 
    
  //   std::cout << "Tex coord offset = [" << proj.tex_coord_offset.x << ", " << proj.tex_coord_offset.y << "]" << std::endl;
  // }



  return 0;
}

