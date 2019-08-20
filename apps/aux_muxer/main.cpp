// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/types.h>
#include <lamure/prov/auxi.h>
#include <lamure/prov/octree.h>

#include <scm/core/math.h>
#include <scm/gl_core/math.h>

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <iomanip>

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

struct cameraIntrinsic {
  int32_t cameraId_;
  float focalValueH_;
  float reserved_0_;
  float reserved_1_;
  float reserved_2_;
  float focalValueV_;
  float reserved_3_;
  float centerX_;
  float centerY_;
  float reserved_4_;
  int32_t imageWidth_;
  int32_t imageHeight_;
};

struct cameraDistortion {
  int32_t cameraId_;
  float radial1_;
  float radial2_;
  float radial3_;
  float tangential1_;
  float tangential2_;
};

struct cameraPose {
  int32_t cameraId_;
  float positionX_;
  float positionY_;
  float positionZ_;
  float rot11_;
  float rot12_;
  float rot13_;
  float rot21_;
  float rot22_;
  float rot23_;
  float rot31_;
  float rot32_;
  float rot33_;
};

struct worldPointDetectionError {
  int32_t worldPointId_;
  int32_t cameraId_;
  int32_t featureId_;
  float projectErrorX_;
  float projectErrorY_;
  float projectErrorLen_;
};

struct feature {
  int32_t cameraId_;
  int32_t featureId_;
  int32_t usingCount_;
  float featureX_;
  float featureY_;
  float featureSize_;
  float featureAngle_;
};



int main(int argc, char *argv[]) {
    if (argc == 1 || 
      cmd_option_exists(argv, argv+argc, "-h") ||
      !cmd_option_exists(argv, argv+argc, "-f")) {
      std::cout << "Usage: " << argv[0] << std::endl <<
         "INFO: aux_import " << std::endl <<
         "\t-f input folder (required)" << std::endl <<
         "\t-a auxi output file (default: default.auxi)" << std::endl <<
         "\t(OPTIONAL: -x -y -z to define transformation, default: 0 0 0)" << std::endl <<
         std::endl; 
      return 0;
    }

    double transform_x = 0.0;
    double transform_y = 0.0;
    double transform_z = 0.0;

    std::string input_folder = std::string(get_cmd_option(argv, argv + argc, "-f"));
    if (input_folder[input_folder.size()-1] != '/') {
      input_folder += "/";
    }

    std::string aux_file = input_folder + "default.auxi";
    if (cmd_option_exists(argv, argv+argc, "-a")) {
      aux_file = input_folder + std::string(get_cmd_option(argv, argv + argc, "-a"));
    }

    if (cmd_option_exists(argv, argv+argc, "-x")) {
      transform_x = atof(std::string(get_cmd_option(argv, argv + argc, "-x")).c_str());
    }
    if (cmd_option_exists(argv, argv+argc, "-y")) {
      transform_y = atof(std::string(get_cmd_option(argv, argv + argc, "-y")).c_str());
    }
    if (cmd_option_exists(argv, argv+argc, "-z")) {
      transform_z = atof(std::string(get_cmd_option(argv, argv + argc, "-z")).c_str());
    }

    //parse cameraNames.txt
    std::map<int32_t, cameraName> cameraNames;
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
        cameraNames[cN.cameraId_] = cN;
      }
    }
    cameraNames_file.close();

    uint64_t num_views = cameraNames.size();
    std::cout << "cameraNames.txt " << num_views << " cameras found" << std::endl; 


    //parse cameraIntrinsics.txt
    std::map<int32_t, cameraIntrinsic> cameraIntrinsics;
    std::string cameraIntrinsics_filepath = input_folder + "cameraIntrinsics.txt";
    
    std::ifstream cameraIntrinsics_file(cameraIntrinsics_filepath, std::ios::in);
    if(cameraIntrinsics_file.is_open()) {
      std::cout << cameraIntrinsics_filepath << std::endl;
    }
    else {
      std::cout << "File not found: " << cameraIntrinsics_filepath << std::endl;
      exit(0);
    }

    while (std::getline(cameraIntrinsics_file, line)) {
      if (line.length() > 1) {
        if (line[0] == '/' && line[1] == '/') {
          continue;
        }
        cameraIntrinsic cI;
        std::istringstream line_ss(line);
        line_ss >> cI.cameraId_;
        line_ss >> cI.focalValueH_;
        line_ss >> cI.reserved_0_;
        line_ss >> cI.reserved_1_;
        line_ss >> cI.reserved_2_;
        line_ss >> cI.focalValueV_;
        line_ss >> cI.reserved_3_;
        line_ss >> cI.centerX_;
        line_ss >> cI.centerY_;
        line_ss >> cI.reserved_4_;
        line_ss >> cI.imageWidth_;
        line_ss >> cI.imageHeight_;
        cameraIntrinsics[cI.cameraId_] = cI;
      }
    }
    cameraIntrinsics_file.close();

    std::cout << "cameraIntrinsics.txt " << cameraIntrinsics.size() << " cameras found" << std::endl; 



    //parse cameraDistortions.txt
    std::map<int32_t, cameraDistortion> cameraDistortions;
    std::string cameraDistortions_filepath = input_folder + "cameraDistortions.txt";
    
    std::ifstream cameraDistortions_file(cameraDistortions_filepath, std::ios::in);
    if(cameraDistortions_file.is_open()) {
      std::cout << cameraDistortions_filepath << std::endl;
    }
    else {
      std::cout << "File not found: " << cameraDistortions_filepath << std::endl;
      exit(0);
    }

    while (std::getline(cameraDistortions_file, line)) {
      if (line.length() > 1) {
        if (line[0] == '/' && line[1] == '/') {
          continue;
        }
        cameraDistortion cD;
        std::istringstream line_ss(line);
        line_ss >> cD.cameraId_;
        line_ss >> cD.radial1_;
        line_ss >> cD.radial2_;
        line_ss >> cD.radial3_;
        line_ss >> cD.tangential1_;
        line_ss >> cD.tangential2_;
        cameraDistortions[cD.cameraId_] = cD;
      }
    }
    cameraDistortions_file.close();

    std::cout << "cameraDistortions.txt " << cameraDistortions.size() << " cameras found" << std::endl; 



    //parse cameraPoses.txt
    std::map<int32_t, cameraPose> cameraPoses;
    std::string cameraPoses_filepath = input_folder + "cameraPoses.txt";
    
    std::ifstream cameraPoses_file(cameraPoses_filepath, std::ios::in);
    if(cameraPoses_file.is_open()) {
      std::cout << cameraPoses_filepath << std::endl;
    }
    else {
      std::cout << "File not found: " << cameraPoses_filepath << std::endl;
      exit(0);
    }

    while (std::getline(cameraPoses_file, line)) {
      if (line.length() > 1) {
        if (line[0] == '/' && line[1] == '/') {
          continue;
        }
        cameraPose cP;
        std::istringstream line_ss(line);
        line_ss >> cP.cameraId_;

        double pos;
        line_ss >> std::setprecision(32) >> pos;
        pos += transform_x; cP.positionX_ = (float)pos;
        line_ss >> std::setprecision(32) >> pos;
        pos += transform_y; cP.positionY_ = (float)pos;
        line_ss >> std::setprecision(32) >> pos;
        pos += transform_z; cP.positionZ_ = (float)pos;
        
        line_ss >> cP.rot11_;
        line_ss >> cP.rot12_;
        line_ss >> cP.rot13_;
        line_ss >> cP.rot21_;
        line_ss >> cP.rot22_;
        line_ss >> cP.rot23_;
        line_ss >> cP.rot31_;
        line_ss >> cP.rot32_;
        line_ss >> cP.rot33_;
        cameraPoses[cP.cameraId_] = cP;
      }
    }
    cameraPoses_file.close();

    std::cout << "cameraPoses.txt " << cameraPoses.size() << " cameras found" << std::endl; 



    //parse worldPoints.txt
    std::map<int32_t, worldPoint> worldPoints;
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

        double pos;
        line_ss >> std::setprecision(32) >> pos;
        pos += transform_x; wP.worldPointX_ = (float)pos;
        line_ss >> std::setprecision(32) >> pos;
        pos += transform_y; wP.worldPointY_ = (float)pos;
        line_ss >> std::setprecision(32) >> pos;
        pos += transform_z; wP.worldPointZ_ = (float)pos;
        
        worldPoints[wP.worldPointId_] = wP;
      }
    }
    worldPoints_file.close();

    uint64_t num_points = worldPoints.size();
    std::cout << "worldPoints.txt " << num_points << " points found" << std::endl; 



    //parse worldPointDetectionError.txt
    std::map<int32_t, std::map<int32_t, std::map<int32_t, worldPointDetectionError>>> worldPointDetectionErrors;
    std::string worldPointDetectionError_filepath = input_folder + "worldPointDetectionError.txt";
    
    std::ifstream worldPointDetectionError_file(worldPointDetectionError_filepath, std::ios::in);
    if(worldPointDetectionError_file.is_open()) {
      std::cout << worldPointDetectionError_filepath << std::endl;
    }
    else {
      std::cout << "File not found: " << worldPointDetectionError_filepath << std::endl;
      exit(0);
    }

    while (std::getline(worldPointDetectionError_file, line)) {
      if (line.length() > 1) {
        if (line[0] == '/' && line[1] == '/') {
          continue;
        }
        worldPointDetectionError wPDE;
        std::istringstream line_ss(line);
        line_ss >> wPDE.worldPointId_;
        line_ss >> wPDE.cameraId_;
        line_ss >> wPDE.featureId_;
        line_ss >> wPDE.projectErrorX_;
        line_ss >> wPDE.projectErrorY_;
        line_ss >> wPDE.projectErrorLen_;
        worldPointDetectionErrors[wPDE.worldPointId_][wPDE.cameraId_][wPDE.featureId_] = wPDE;
      }
    }
    worldPointDetectionError_file.close();

    std::cout << "worldPointDetectionError.txt " << worldPointDetectionErrors.size() << " entries found" << std::endl; 



    //parse features.txt
    std::map<int32_t, std::map<int32_t, feature>> features;
    std::string features_filepath = input_folder + "features.txt";
    
    std::ifstream features_file(features_filepath, std::ios::in);
    if(features_file.is_open()) {
      std::cout << features_filepath << std::endl;
    }
    else {
      std::cout << "File not found: " << features_filepath << std::endl;
      exit(0);
    }

    while (std::getline(features_file, line)) {
      if (line.length() > 1) {
        if (line[0] == '/' && line[1] == '/') {
          continue;
        }
        feature f;
        std::istringstream line_ss(line);
        line_ss >> f.cameraId_;
        line_ss >> f.featureId_;
        line_ss >> f.usingCount_;
        line_ss >> f.featureX_;
        line_ss >> f.featureY_;
        line_ss >> f.featureSize_;
        line_ss >> f.featureAngle_;
        features[f.cameraId_][f.featureId_] = f;
      }
    }
    features_file.close();

    std::cout << "features.txt " << features.size() << " entries found" << std::endl; 



    //write auxi object
    lamure::prov::auxi aux;

    for (auto it : cameraNames) {
      auto& cN = it.second;

      int32_t id = cN.cameraId_;
      auto& cI = cameraIntrinsics.find(id)->second;
      auto& cD = cameraDistortions.find(id)->second;
      auto& cP = cameraPoses.find(id)->second;

      lamure::prov::auxi::view v;
      v.camera_id_ = cN.cameraId_;
      
#if 0
      v.image_file_ = std::to_string(cN.cameraId_);
      while (v.image_file_.size() < 8) {
        v.image_file_ = "0" + v.image_file_;
      }
      v.image_file_ += ".jpg";
#else
      v.image_file_ = cN.cameraName_;
      v.image_file_ += ".jpg"; 
#endif     
      v.position_ = scm::math::vec3f(cP.positionX_, cP.positionY_, cP.positionZ_);
      scm::math::mat4f m = scm::math::mat4f::identity();
      m.m00 = cP.rot11_; m.m01 = cP.rot12_; m.m02 = cP.rot13_;
      m.m04 = cP.rot21_; m.m05 = cP.rot22_; m.m06 = cP.rot23_;
      m.m08 = cP.rot31_; m.m09 = cP.rot32_; m.m10 = cP.rot33_;
      v.transform_ = scm::math::make_translation(v.position_)
        * scm::math::transpose(m) * scm::math::make_rotation(180.f, scm::math::vec3f(0, 1, 0));

      v.focal_length_ = 0.f;
      v.distortion_ = 0.f;
      v.image_width_ = cI.imageWidth_;
      v.image_height_ = cI.imageHeight_;
      v.atlas_tile_id_ = 0;    
      
      aux.add_view(v);
    }

    for (auto it : worldPoints) {
      auto& wP = it.second;

      int32_t id = wP.worldPointId_;
      auto& wPDE_maps = worldPointDetectionErrors.find(id)->second;
      
      lamure::prov::auxi::sparse_point p;

      p.pos_ = scm::math::vec3f(wP.worldPointX_, wP.worldPointY_, wP.worldPointZ_);
      p.r_ = (uint8_t)255;
      p.g_ = (uint8_t)255;
      p.b_ = (uint8_t)255;
      p.a_ = (uint8_t)255;
      
      auto aux_features = std::vector<lamure::prov::auxi::feature>();
      for (const auto& wPDE_map : wPDE_maps) {
        for (const auto& wPDE : wPDE_map.second) {
          auto& feature = features[wPDE.second.cameraId_][wPDE.second.featureId_];
          
          lamure::prov::auxi::feature f;
          f.camera_id_ = wPDE.second.cameraId_;
          f.using_count_ = feature.usingCount_;
          f.coords_ = scm::math::vec2f(feature.featureX_, feature.featureY_);
          f.error_ = scm::math::vec2f(wPDE.second.projectErrorX_, wPDE.second.projectErrorY_);

          aux_features.push_back(f);
        }
      }
      
      p.features_ = aux_features;

      aux.add_sparse_point(p);
      
    }


    std::cout << "create octree " << std::endl;

    //create octree
    auto octree = std::make_shared<lamure::prov::octree>();
    octree->create(aux.get_sparse_points());
    aux.set_octree(octree);

    std::cout << "Write auxi to file..." << std::endl;
    aux.write_aux_file(aux_file);

    std::cout << "Num views: " << aux.get_num_views() << std::endl;
    std::cout << "Num points: " << aux.get_num_sparse_points() << std::endl;
    std::cout << "Done" << std::endl;

    return 0;
}
