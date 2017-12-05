// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <climits>
#include <limits>
#include <cfloat>
#include <cmath>
#include <vector>

#include <cstdlib>


#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#define BOOST_FILESYSTEM_VERSION 3
#define BOOST_FILESYSTEM_NO_DEPRECATED 
#include <boost/filesystem.hpp>
using namespace std;
namespace fs = ::boost::filesystem;


// return the filenames of all files that have the specified extension
// in the specified directory and all subdirectories
void get_all(const fs::path& root, const string& ext, vector<fs::path>& ret)
{
    if(!fs::exists(root) || !fs::is_directory(root)) return;

    fs::recursive_directory_iterator it(root);
    fs::recursive_directory_iterator endit;

    while(it != endit)
    {
        if(fs::is_regular_file(*it) && it->path().extension() == ext) ret.push_back(it->path().filename());
        ++it;

    }

}

char* get_cmd_option(char** begin, char** end, const std::string & option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}


int main(int argc, char** argv) {

  if (argc == 1 || 
    !cmd_option_exists(argv, argv+argc, "-in") ||
    !cmd_option_exists(argv, argv+argc, "-patch") ||
    !cmd_option_exists(argv, argv+argc, "-ply")) {
        
    std::cout << "Usage: " << argv[0] << "<flags> -f <input_file>" << std::endl <<
       "INFO: bvh_leaf_extractor " << std::endl <<
       "\t-in: selects input folder (.cmvs/00/models)" << std::endl <<
       "\t-patch: select output patch file" << std::endl <<
       "\t-ply: select output ply file" << std::endl <<
       std::endl;
    return 0;
  }

  std::string out_patch_file = std::string(get_cmd_option(argv, argv + argc, "-patch"));
  std::string out_ply_file = std::string(get_cmd_option(argv, argv + argc, "-ply"));

  std::string in_folder = std::string(get_cmd_option(argv, argv + argc, "-in"));
  
  fs::path in_path = in_folder;
  std::string ply_ext = ".ply";
  std::vector<fs::path> in_ply_files;
  std::cout << "scanning for ply in " << in_folder << std::endl;
  get_all(in_path, ply_ext, in_ply_files);
  std::sort(in_ply_files.begin(), in_ply_files.end());
  for (const auto& f : in_ply_files) {
    std::cout << f.string() << std::endl;
  }
  std::cout << in_ply_files.size() << " ply files found" << std::endl;

  std::string patch_ext = ".patch";
  std::vector<fs::path> in_patch_files;
  std::cout << "scanning for patch in " << in_folder << std::endl;
  get_all(in_path, patch_ext, in_patch_files);
  std::sort(in_patch_files.begin(), in_patch_files.end());
  for (const auto& f : in_patch_files) {
    std::cout << f.string() << std::endl;
  }
  std::cout << in_patch_files.size() << " patch files found" << std::endl;

  //write .ply file
  //count num points
  uint64_t num_points = 0;

  for (const auto file : in_ply_files) {
    std::string f = file.string();
    std::string filepath = in_path.string() + f.substr(0, f.size());
    //std::cout << filepath << std::endl;

    std::string line;
    std::ifstream myfile(filepath.c_str());

    while (std::getline(myfile, line)) {
        ++num_points;
    }
    num_points -= 13;
    myfile.close();
  }
  std::cout << num_points << " points" << std::endl;

  //write concat ply
  std::cout << "CONCAT: " << out_ply_file << std::endl;

  ofstream ply_out(out_ply_file.c_str());

  ply_out << "ply\n";
  ply_out << "format ascii 1.0\n";
  ply_out << "element vertex " << num_points << "\n";
  ply_out << "property float x\n";
  ply_out << "property float y\n";
  ply_out << "property float z\n";
  ply_out << "property float nx\n";
  ply_out << "property float ny\n";
  ply_out << "property float nz\n";
  ply_out << "property uchar diffuse_red\n";
  ply_out << "property uchar diffuse_green\n";
  ply_out << "property uchar diffuse_blue\n";
  ply_out << "end_header\n";
  for (const auto file : in_ply_files) {
    std::string f = file.string();
    std::string filepath = in_path.string() + f.substr(0, f.size());
    
    std::string line;
    std::ifstream myfile(filepath.c_str());

    uint32_t count = 0;
    while (std::getline(myfile, line)) {
      if (++count > 13) { //exclude ply header
        ply_out << line << "\n";
      }
    }
    myfile.close();
  }

  ply_out.close();
  std::cout << "done." << std::endl;

  //write concat patch
  std::cout << "CONCAT: " << out_patch_file << std::endl;

  ofstream patch_out(out_patch_file.c_str());

  patch_out << "PATCHES\n";
  patch_out << num_points << "\n";

  for (const auto file : in_patch_files) {
    std::string f = file.string();
    std::string filepath = in_path.string() + f.substr(0, f.size());
    
    std::string line;
    std::ifstream myfile(filepath.c_str());

    uint32_t count = 0;
    while (std::getline(myfile, line)) {
      if (++count > 2) { //exclude patch header
        patch_out << line << "\n";
      }
    }
    myfile.close();
  }
  
  patch_out.close();
  std::cout << "done." << std::endl;


  return 0;

}


