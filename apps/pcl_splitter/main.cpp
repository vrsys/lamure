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
  
  double splitting_distance = 1.0;

  if (cmd_option_exists(argv, argv + argc, "-i")) {
    input_filename = std::string(get_cmd_option(argv, argv + argc, "-i"));
  }
  else terminate = true;

  if (cmd_option_exists(argv, argv + argc, "-f")) {
    splitting_distance = atof(get_cmd_option(argv, argv + argc, "-f"));
  }
  else terminate = true;

  if (terminate) {
    std::cout << "Usage: " << argv[0] << "<flags>\n" <<
      "INFO: " << argv[0] << "\n" <<
      "\t-i: select input .txt file with all .xyz files line by line\n" <<
      "\t-f: select splitting distance (effectively the edge length of one cell)\n" << 
      std::endl;
    std::exit(0);
  }
  
  //load all xyz filenames
  std::vector<std::string> xyz_filenames;

  {
    std::ifstream input_file(input_filename.c_str());

    std::string line;
    while (getline(input_file, line)) {

      line.erase(std::remove(line.begin(),line.end(),' '), line.end());
      xyz_filenames.push_back(line);
    }

    input_file.close();
  }

  //loop all input files to determine the global bounding box

  scm::math::vec3d box_min(std::numeric_limits<double>::max());
  scm::math::vec3d box_max(std::numeric_limits<double>::lowest());

  std::cout << "Starting 1st pass ..." << std::endl;

  bool xyz_all = false;

  for (uint32_t i = 0; i < xyz_filenames.size(); i++) {

    std::string xyz_filename = xyz_filenames[i];
    std::cout << "Open file " << xyz_filename << " ..." << std::endl;

    xyz_all = (input_filename.substr(input_filename.size()-8) == ".xyz_all");
    if (!xyz_all) {
      if (input_filename.substr(input_filename.size()-4) != ".xyz") {
        std::cout << "ERROR: Invalid input format. Expected .xyz or .xyz_all" << std::endl;
        std::exit(1);
      }
    }

    std::ifstream xyz_file(xyz_filename.c_str());

    std::cout << "Expaning bounding box..." << std::endl;
  
    //iterate all points

    std::string line;
    while(getline(xyz_file, line)) {

      std::istringstream lineparser(line);

      scm::math::vec3d pos;
      lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.x;
      lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.y;
      lineparser >> std::setprecision(DEFAULT_PRECISION) >> pos.z;

      box_min.x = std::min(box_min.x, pos.x);
      box_min.y = std::min(box_min.y, pos.y);
      box_min.z = std::min(box_min.z, pos.z);

      box_max.x = std::max(box_max.x, pos.x);
      box_max.y = std::max(box_max.y, pos.y);
      box_max.z = std::max(box_max.z, pos.z);

    }

    xyz_file.close();

    std::cout << "\tcurrent min: " << box_min << std::endl;
    std::cout << "\tcurrent max: " << box_max << std::endl;

  }

  
  //determine subdivision along all axis

  auto box_dim = box_max - box_min;

  scm::math::vec3ui num_cells_per_axis;
  num_cells_per_axis.x = (uint32_t)std::ceil(box_dim.x / splitting_distance);
  num_cells_per_axis.y = (uint32_t)std::ceil(box_dim.y / splitting_distance);
  num_cells_per_axis.z = (uint32_t)std::ceil(box_dim.z / splitting_distance);

  std::cout << "Num cells: " << num_cells_per_axis << std::endl;
  uint32_t num_cells = num_cells_per_axis.x * num_cells_per_axis.y * num_cells_per_axis.z;
  
  //create and truncate all output file stream

  std::cout << "Creating " << num_cells << " output files ..." << std::endl;

  std::vector<std::string> output_filenames;
  std::vector<std::ofstream> output_files;

  for (uint32_t i = 0; i < num_cells; ++i) {
    if (xyz_all) {
      output_filenames.push_back(input_filename.substr(0, input_filename.size()-4) + "_cell_" + std::to_string(i) + ".xyz_all");
    }
    else {
      output_filenames.push_back(input_filename.substr(0, input_filename.size()-4) + "_cell_" + std::to_string(i) + ".xyz");
    }

    output_files.push_back(std::ofstream(output_filenames.back().c_str(), std::ios::trunc));
  }
  
  std::cout << "Starting 2nd pass ..." << std::endl;

  //iterate all input files

  for (uint32_t i = 0; i < xyz_filenames.size(); i++) {

    std::string xyz_filename = xyz_filenames[i];
    std::cout << "Open file " << xyz_filename << " ..." << std::endl;

    xyz_all = (input_filename.substr(input_filename.size()-8) == ".xyz_all");
    if (!xyz_all) {
      if (input_filename.substr(input_filename.size()-4) != ".xyz") {
        std::cout << "ERROR: Invalid input format. Expected .xyz or .xyz_all" << std::endl;
        std::exit(1);
      }
    }
  
    std::ifstream xyz_file(xyz_filename.c_str());

    //iterate all points

    std::cout << "Flushing points ..." << std::endl;
 
    std::string line;
    while(getline(xyz_file, line)) {

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

      //store surfel into ouput files based on local position

      pos -= box_min;

      scm::math::vec3ui cell;
      cell.x = (uint32_t)std::ceil(pos.x / splitting_distance);
      cell.y = (uint32_t)std::ceil(pos.y / splitting_distance);
      cell.z = (uint32_t)std::ceil(pos.z / splitting_distance);

      uint32_t file_idx = cell.x 
                        + cell.y * num_cells_per_axis.x
                        + cell.z * (num_cells_per_axis.x*num_cells_per_axis.y);

      output_files[file_idx] << std::setprecision(DEFAULT_PRECISION) << surfel.pos().x << " ";
      output_files[file_idx] << std::setprecision(DEFAULT_PRECISION) << surfel.pos().y << " ";
      output_files[file_idx] << std::setprecision(DEFAULT_PRECISION) << surfel.pos().z << " ";

      if (xyz_all) {
        output_files[file_idx] << std::setprecision(DEFAULT_PRECISION) << surfel.normal().x << " ";
        output_files[file_idx] << std::setprecision(DEFAULT_PRECISION) << surfel.normal().y << " ";
        output_files[file_idx] << std::setprecision(DEFAULT_PRECISION) << surfel.normal().z << " ";
      }

      output_files[file_idx] << (int)surfel.color().x << " ";
      output_files[file_idx] << (int)surfel.color().y << " ";
      output_files[file_idx] << (int)surfel.color().z << " ";

      if (xyz_all) {
        output_files[file_idx] << std::setprecision(DEFAULT_PRECISION) << surfel.radius() << " ";
      }

      output_files[file_idx] << std::endl;


    }    

    xyz_file.close();
 

  }

  for (auto& of : output_files) {
    of.close();
  }

  //done

  std::cout << "Done. Have a nice day." << std::endl;

  
  return 0;

}


