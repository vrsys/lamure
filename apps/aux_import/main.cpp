// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/types.h>
#include <lamure/prov/aux.h>

#include <scm/core/math.h>
#include <scm/gl_core/math.h>

#include <iostream>
#include <string>

using namespace std;

char *get_cmd_option(char **begin, char **end, const string &option) {
    char **it = find(begin, end, option);
    if(it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char **begin, char **end, const string &option) { 
    return find(begin, end, option) != end; 
}

bool check_file_extensions(string name_file, const char *pext) {
    string ext(pext);
    if (name_file.substr(name_file.size() - ext.size()).compare(ext) != 0) {
        cout << "Please specify " + ext + " file as input" << endl;
        return true;
    }
    return false;
}

struct cameraName {
  int32_t cameraId_;
  std::string cameraName_;
};

struct worldPoint {
  int32_t worldPointId_;
  float worldPointX_;
  float worldPointY_;
  float worldPointZ_;
};



int main(int argc, char *argv[]) {
    if (argc == 1 || 
      cmd_option_exists(argv, argv+argc, "-h") ||
      !cmd_option_exists(argv, argv+argc, "-f")) {
      std::cout << "Usage: " << argv[0] << std::endl <<
         "INFO: aux_import " << std::endl <<
         "\t-f input folder (required)" << std::endl <<
         "\t-a aux output file (default: default.aux)" << std::endl <<
         std::endl;
      return 0;
    }

    std::string input_folder = std::string(get_cmd_option(argv, argv + argc, "-f"));
    if (input_folder[input_folder.size()-1] != '/') {
      input_folder += "/";
    }

    std::string aux_file = input_folder + "default.aux";
    if (cmd_option_exists(argv, argv+argc, "-a")) {
      aux_file = input_folder + std::string(get_cmd_option(argv, argv + argc, "-a"));
    }

    //parse cameraNames.txt
    std::vector<cameraName> cameraNames;
    std::string cameraNames_filepath = input_folder + "cameraNames.txt";
    
    std::ifstream cameraNames_file(cameraNames_filepath, std::ios::in);
    if(cameraNames_file.is_open()) {
      std::cout << cameraNames_filepath << std::endl;
    }
    else {
      std::cout << "File not found: " << cameraNames_filepath << std::endl;
      exit(0);
    }

    std::string line;
    while (std::getline(cameraNames_file, line)) {
      if (line.length() > 1) {
        if (line[0] == '/' && line[1] == '/') {
          continue;
        }
        cameraName cN;
        std::istringstream line_ss(line);
        line_ss >> cN.cameraId_;
        line_ss >> cN.cameraName_;
        cameraNames.push_back(cN);
      }
    }
    cameraNames_file.close();

    std::cout << "cameraNames.txt " << cameraNames.size() << " cameras found" << std::endl; 

    //parse worldPoints.txt
    std::vector<worldPoint> worldPoints;
    std::string worldPoints_filepath = input_folder + "worldPoints.txt";
    
    std::ifstream worldPoints_file(worldPoints_filepath, std::ios::in);
    if(worldPoints_file.is_open()) {
      std::cout << worldPoints_filepath << std::endl;
    }
    else {
      std::cout << "File not found: " << worldPoints_filepath << std::endl;
      exit(0);
    }

    while (std::getline(worldPoints_file, line)) {
      if (line.length() > 1) {
        if (line[0] == '/' && line[1] == '/') {
          continue;
        }
        worldPoint wP;
        std::istringstream line_ss(line);
        line_ss >> wP.worldPointId_;
        line_ss >> wP.worldPointX_;
        line_ss >> wP.worldPointY_;
        line_ss >> wP.worldPointZ_;
        worldPoints.push_back(wP);
      }
    }
    worldPoints_file.close();

    std::cout << "worldPoints.txt " << worldPoints.size() << " points found" << std::endl; 



    //write aux object
    lamure::prov::aux aux;

    for (uint64_t i = 0; i < cameraNames.size(); ++i) {
      auto& cN = cameraNames[i];

      lamure::prov::aux::view v;
      v.camera_id_ = cN.cameraId_;
      v.image_file_ = std::to_string(cN.cameraId_);
      while (v.image_file_.size() < 8) {
        v.image_file_ = "0" + v.image_file_;
      }
      v.image_file_ += ".jpg";
     
      v.position_ = scm::math::vec3f(0.f);
      v.transform_ = scm::math::mat4f::identity();
      v.focal_length_ = 0.f;
      v.distortion_ = 0.f;
      v.image_width_ = 0;
      v.image_height_ = 0;
      v.tex_atlas_id_ = 0;    
      
      aux.add_view(v);
    }


    for (uint64_t i = 0; i < worldPoints.size(); ++i) {
      auto& wP = worldPoints[i];

      lamure::prov::aux::sparse_point p;

      p.pos_ = scm::math::vec3f(wP.worldPointX_, wP.worldPointY_, wP.worldPointZ_);
      p.r_ = (uint8_t)255;
      p.g_ = (uint8_t)255;
      p.b_ = (uint8_t)255;
      p.a_ = (uint8_t)255;
      p.features_ = std::vector<lamure::prov::aux::feature>();

      aux.add_sparse_point(p);
      
    }


    std::cout << "Write aux to file..." << std::endl;
    aux.write_aux_file(aux_file);

    std::cout << "Num views: " << aux.get_num_views() << std::endl;
    std::cout << "Num points: " << aux.get_num_sparse_points() << std::endl;
    std::cout << "Done" << std::endl;

    return 0;
}
