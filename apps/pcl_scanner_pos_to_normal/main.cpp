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

#include "cgal.h"

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

    if (xyz_list_line.size() <= 9) {
      xyz_list_line = "";
      continue;
    }

    xyz_filenames.push_back(xyz_list_line);
    xyz_list_line = "";
  }

  xyz_list_file.close();


  std::vector<scm::math::vec3d> scanner_positions;

  std::ifstream pos_list_file(pos_list_filename.c_str());
  std::string pos_list_line;
  while (getline(pos_list_file, pos_list_line)) {

    if (pos_list_line.size() <= 9) {
      pos_list_line = "";
      continue;
    }

    std::istringstream lineparser(pos_list_line);

    std::string dummy;
    //std::cout << "line: " << dummy << std::endl;

    scm::math::vec3d pos(0.0);

    lineparser >> dummy;
    lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.x;
    lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.y;
    lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.z;

    scanner_positions.push_back(pos);
    pos_list_line = "";
  }

  pos_list_file.close();


  std::cout << xyz_filenames.size() << " xyz filenames loaded." << std::endl;

  std::cout << scanner_positions.size() << " scanner positions loaded." << std::endl;


/*
  for (const auto& pos : scanner_positions) {
    std::cout << pos << std::endl;
  }
*/

  for (uint32_t i = 0; i < xyz_filenames.size(); ++i) {
    const auto& xyz_file = xyz_filenames[i];

    std::vector<lamure::pre::surfel> pointcloud;    

    std::cout << "Loading pointcloud: " << xyz_file << std::endl;
    load_pointcloud(xyz_file, pointcloud);
    std::cout << pointcloud.size() << " points loaded." << std::endl;

    std::cout << "Computing normals..." << std::endl;
    compute_normals(pointcloud);

    std::cout << "Face normals to scanner..." << std::endl;
    std::cout << "Scanner position: " << scanner_positions[i] << std::endl;
    face_normals_to_scanner(scanner_positions[i], pointcloud);

    //std::string output_file = xyz_file.substr(0, xyz_file.size()-4) + "_facing.xyz_all";
    std::string output_file = pos_list_filename.substr(0, pos_list_filename.size()-4) + "_facing_" + std::to_string(i) + ".xyz_all";

    std::cout << "Writing pointcloud: " << output_file << std::endl;
    write_pointcloud(output_file, pointcloud);

    pointcloud.clear();

    std::cout << "Quit" << std::endl;
    break;

  }




  return 0;

}


