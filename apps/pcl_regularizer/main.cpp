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


int main(int argc, char *argv[]) {

  bool terminate = false;
  std::string input_filename = "";
  double regularization_distance = 0.1;

  if (cmd_option_exists(argv, argv + argc, "-i")) {
    input_filename = std::string(get_cmd_option(argv, argv + argc, "-i"));
  }
  else terminate = true;

  //define regularization distance between points
  if (cmd_option_exists(argv, argv + argc, "-r")) {
    regularization_distance = atof(get_cmd_option(argv, argv + argc, "-r"));
  }
  else terminate = true;

  if (terminate) {
    std::cout << "Usage: " << argv[0] << "<flags>\n" <<
      "INFO: " << argv[0] << "\n" <<
      "\t-i: select input .xyz file\n" <<
      "\t-r: select regularization distance between surfels\n" << std::endl;
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
    if (xyz_all) {
    	lineparser >> std::setprecision(DEFAULT_PRECISION) >> radius;
    }

 
    lamure::pre::surfel surfel(pos, bcolor, radius, normal, 0.0);
    input_surfels.push_back(surfel);

  }

  input_file.close();

  std::cout << input_surfels.size() << " points loaded." << std::endl;

  //container for regularized result
  std::vector<lamure::pre::surfel> output_surfels;

  if (input_surfels.size() > 10) {

    //do the thing!
    std::shared_ptr<lamure::pre::octree> octree = std::make_shared<lamure::pre::octree>();
    octree->regularize(input_surfels, regularization_distance, output_surfels);
    octree.reset();

    if (output_surfels.size() < 10) {
      std::cout << "Too few surfels left after regularization." << std::endl;
      return 0;
    }

  }
  else {
    std::cout << "Too few input surfels. Let's just skip these." << std::endl;
    return 0;
  }



  std::string output_filename = input_filename.substr(0, input_filename.size()-4) + "_regularized.xyz";

  std::cout << "Writing output file " << output_filename << std::endl;

  std::ofstream output_file(output_filename.c_str(), std::ios::trunc);
  output_file.close();

  line = "";
  for (uint64_t i = 0; i < output_surfels.size(); ++i) {
    get_string(output_surfels[i], line, xyz_all);

    if (((i % 1000000) == 0) || (i == output_surfels.size()-1)) {

      std::ofstream output_file(output_filename.c_str(), std::ios::app);
      output_file << line;
      output_file.close();

      line = "";
    }
  }

  output_surfels.clear();

  std::cout << "Done. Have a nice day." << std::endl;


  return 0;

}


