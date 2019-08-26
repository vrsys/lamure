// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <memory>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

#include <lamure/types.h>
#include <scm/core/math.h>

#include <lamure/pre/surfel.h>
#include <lamure/pre/octree.h>

#define DEFAULT_PRECISION 15

static char *get_cmd_option(char **begin, char **end, const std::string &option) {
    char **it = std::find(begin, end, option);
    if (it != end && ++it != end) {
        return *it;
    }
    return 0;
}

static bool cmd_option_exists(char **begin, char **end, const std::string &option) {
    return std::find(begin, end, option) != end;
}



int main(int argc, char *argv[]) {

  bool terminate = false;
  std::string input_filename = "";
  std::string output_filename = "";

  if (cmd_option_exists(argv, argv + argc, "-i")) {
    input_filename = std::string(get_cmd_option(argv, argv + argc, "-i"));
  }
  else terminate = true;

  if (cmd_option_exists(argv, argv + argc, "-o")) {
    output_filename = std::string(get_cmd_option(argv, argv + argc, "-o"));
  }
  else terminate = true;

  if (terminate) {
    std::cout << "Usage: " << argv[0] << "<flags>\n" <<
      "INFO: " << argv[0] << "\n" <<
      "\t-i: select input .xyz file\n" <<
      "\t-o: select output .xyz file\n" << std::endl; 
    std::exit(0);
  }
  
  //load input surfels, can be xyz or xyz_all
  std::vector<lamure::pre::surfel> input_surfels;

  bool xyz_all = (input_filename.substr(input_filename.size()-8) == ".xyz_all");
  if (!xyz_all) {
  	if (input_filename.substr(input_filename.size()-4) != ".xyz") {
  		std::cout << "ERROR: Invalid input format. Expected .xyz or .xyz_all" << std::endl;
  		std::exit(1);
  	}
  }


  std::ifstream input_file(input_filename.c_str());

  std::string line;
  while(getline(input_file, line)) {

    std::istringstream lineparser(line);

    scm::math::vec3d pos;
    lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.x;
    lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.y;
    lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.z;

    if (xyz_all) {
    	//...
    }

    scm::math::vec3d color;
    lineparser >> color.x;
    lineparser >> color.y;
    lineparser >> color.z;

    lamure::vec3b bcolor(color.x, color.y, color.z);
    scm::math::vec3f normal(0);

    if (xyz_all) {
    	//...
    }

 
    lamure::pre::surfel surfel(pos, bcolor, 1.0f, normal, 0.0);
    input_surfels.push_back(surfel);

  }

  input_file.close();

  std::cout << input_surfels.size() << " points loaded." << std::endl;

  

  //define regularization distance between points in meters
  double regularization_distance = 1.0;

  //container for regularized result
  std::vector<lamure::pre::surfel> output_surfels;

  //do the thing!
  std::shared_ptr<lamure::pre::octree> octree = std::make_shared<lamure::pre::octree>();
  octree->regularize(input_surfels, regularization_distance, output_surfels);
  octree.reset();


  std::ofstream output_file(output_filename.c_str(), std::ios::trunc);

  for (const auto& surfel : output_surfels) {
  	output_file << std::setprecision(DEFAULT_PRECISION) << surfel.pos().x << " ";
  	output_file << std::setprecision(DEFAULT_PRECISION) << surfel.pos().y << " ";
  	output_file << std::setprecision(DEFAULT_PRECISION) << surfel.pos().z << " ";
  	output_file << (int)surfel.color().x << " ";
  	output_file << (int)surfel.color().y << " ";
  	output_file << (int)surfel.color().z << std::endl;
  }

  output_file.close();


  return 0;

}


