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
  std::string pos_list_filename = "";
  std::string xyz_list_filename = "";

  if (cmd_option_exists(argv, argv + argc, "-f")) {
    xyz_list_filename = std::string(get_cmd_option(argv, argv + argc, "-f"));
  }
  else terminate = true;

  if (cmd_option_exists(argv, argv + argc, "-p")) {
    pos_list_filename = std::string(get_cmd_option(argv, argv + argc, "-p"));
  }
  else terminate = true;

  if (terminate) {
    std::cout << "Usage: " << argv[0] << "<flags>\n" <<
      "INFO: " << argv[0] << "\n" <<
      "\t-f: select pcl list input file (list of .xyz files)\n" <<
      "\t-p: select scanner position list file (list of scanning pos)\n" << std::endl;
    std::exit(0);
  }

  std::vector<std::string> xyz_filenames;

  std::ifstream xyz_list_file(xyz_list_filename.c_str());
  std::string xyz_list_line;
  while(getline(xyz_list_file, xyz_list_line)) {

    if (xyz_list_line.size() <= 3) continue;

    xyz_filenames.push_back(xyz_list_line);
  }

  xyz_list_file.close();


  std::vector<scm::math::vec3d> scanner_positions;

  std::ifstream pos_list_file(pos_list_filename.c_str());
  std::string pos_list_line;
  while (getline(pos_list_file, pos_list_line)) {

    if (pos_list_line.size() <= 3) continue;

    std::istringstream lineparser(pos_list_line);

    std::string dummy;
    //std::cout << "line: " << dummy << std::endl;

    scm::math::vec3d pos(0.0);

    lineparser >> dummy;
    lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.x;
    lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.y;
    lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.z;

    scanner_positions.push_back(pos);
  }

  pos_list_file.close();


  std::cout << xyz_filenames.size() << " xyz filenames loaded." << std::endl;

  std::cout << scanner_positions.size() << " scanner positions loaded." << std::endl;


  for (const auto& xyz_file : xyz_filenames) {
    std::cout << xyz_file << std::endl;
  }

  for (const auto& pos : scanner_positions) {
    std::cout << pos << std::endl;
  }


  return 0;

}


