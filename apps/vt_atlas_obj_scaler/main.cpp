// Copyright (c) 2014-2018 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>
#include <algorithm>
#include <vector>
#include <fstream>
#include <string>
#include <map>
#include <algorithm>

#include <scm/core.h>
#include <scm/core/math.h>

typedef std::vector<std::string> stringvec;

char* get_cmd_option(char** begin, char** end, const std::string & option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

template <class InIt, class OutIt>
void split(InIt begin, InIt end, OutIt splits, char delim) {
  InIt current = begin;
  while (begin != end) {
    if (*begin == delim) {
      *splits++ = std::string(current,begin);
      current = ++begin;
    }
    else
      ++begin;
  }
  *splits++ = std::string(current,begin);
}


struct vertex {
  scm::math::vec3f position_;
  scm::math::vec2f coords_;
  scm::math::vec3f normal_;
};

//load an .obj file and return all coordinates by material
void load_obj_mat(const std::string& _file, std::vector<float>& t, std::map<std::string, std::vector<uint32_t>>& tindices) {
  
  FILE* file = fopen(_file.c_str(), "r");

  std::string current_material = "";


  if (0 != file) {
  
    while (true) {
      char line[128];
      int32_t l = fscanf(file, "%s", line);
      
      if (l == EOF) break;

      if (strcmp(line, "usemtl") == 0) {
        char name[128];
        fscanf(file, "%s\n", name);
        current_material = std::string(name);
        std::cout << "switch material: " << current_material << std::endl;
      }
      else if (strcmp(line, "v") == 0) {

      }
      else if (strcmp(line, "vn") == 0) {

      }
      else if (strcmp(line, "vt") == 0) {
        float tx, ty;
        fscanf(file, "%f %f\n", &tx, &ty);
        t.insert(t.end(), {tx, ty});
      }
      else if (strcmp(line, "f") == 0) {
        std::string vertex1, vertex2, vertex3;
        uint32_t index[3];
        uint32_t coord[3];
        uint32_t normal[3];
        fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", 
          &index[0], &coord[0], &normal[0], 
          &index[1], &coord[1], &normal[1], 
          &index[2], &coord[2], &normal[2]);
        
        tindices[current_material].insert(tindices[current_material].end(), {coord[0], coord[1], coord[2]});
      }
    }

    fclose(file);
/*
    for (const auto& material : tindices) {
      std::string mat = material.first;
      std::cout << "material: " << mat << std::endl;
      std::cout << "coords: " << tindices[mat].size() << std::endl;
    }
*/


  }
  else {
    std::cout << "failed to open file: " << _file << std::endl;
    exit(1);
  }

}


int main(int argc, char *argv[]) {

    if (argc == 1 || 
      !cmd_option_exists(argv, argv+argc, "-obj") ||
      !cmd_option_exists(argv, argv+argc, "-log")) {
      std::cout << "Usage: " << argv[0] << " -obj <input_mesh.obj> -log <atlas.log>" << std::endl <<
         "INFO: argv[0] " << std::endl ;
      return 0;
    }
    
    std::string in_obj_file = get_cmd_option(argv, argv+argc, "-obj");
    std::string in_log_file = get_cmd_option(argv, argv+argc, "-log");

    std::string out_obj_file = in_obj_file.substr(0, in_obj_file.size()-4)+"_vt.obj";
    std::string out_mtl_file = in_obj_file.substr(0, in_obj_file.size()-4)+"_vt.mtl";

    //parse .mtl file
    std::string in_mtl_file = in_obj_file.substr(0, in_obj_file.size()-4)+".mtl";
    std::cout << "loading .mtl file ..." << std::endl;
    std::ifstream mtl_file(in_mtl_file.c_str());
    if (!mtl_file.is_open()) {
      std::cout << "could not open .mtl file" << std::endl;
      exit(-1);
    }

    std::map<std::string, std::string> material_map;

    std::string current_material = "";

    std::string line;
    while (std::getline(mtl_file, line)) {
      if(line.length() >= 2) {
        if (line[0] == '#') {
          continue;
        }
        if (line.substr(0, 6) == "newmtl") {
          current_material = line.substr(7);
          current_material = current_material.substr(0, current_material.size()-1);
          material_map[current_material] = "";
          //std::cout << "switch material: " << current_material << std::endl;
        }
        else if (line.substr(0, 6) == "map_Kd") {
          std::string current_texture = line.substr(7);
          current_texture = current_texture.substr(0, current_texture.size()-1);
          std::cout << current_material << " -> " << current_texture << std::endl;
          material_map[current_material] =  current_texture;
        }
      }
    
    }

    mtl_file.close();


    //parse .log file
    struct tile {
      uint32_t x_;
      uint32_t y_;
      uint32_t width_;
      uint32_t height_;
    };
    std::map<std::string, tile> tile_map;

    std::cout << "loading atlas .log file..." << std::endl;
    std::ifstream log_file(in_log_file.c_str());

    if (!log_file.is_open()) {
      std::cout << "could not open .log file" << std::endl;
      exit(-1);
    }

    uint32_t atlas_width = 0;
    uint32_t atlas_height = 0;
  
    uint32_t line_no = 0;
    while(std::getline(log_file, line)) {
      if(line.length() >= 2) {
        if (line[0] == '#') {
          continue;
        }
        std::vector<std::string> values;
        split(line.begin(), line.end(), std::back_inserter(values), ',');
        if (line_no > 0) {        
          tile t{atoi(values[1].c_str()), atoi(values[2].c_str()), atoi(values[3].c_str()), atoi(values[4].c_str())};
          tile_map[values[values.size()-1]] = t;
          std::cout << values[values.size()-1] << " tile: " << t.x_ << " " << t.y_ << " " << t.width_ << " " << t.height_ << std::endl;
        }
        else {
          atlas_width = atoi(values[0].c_str());
          atlas_height = atoi(values[1].c_str());
        }

      }
      ++line_no;
    }

    log_file.close();

    std::cout << tile_map.size() << " tiles " << std::endl;
    std::cout << "atlas width: " << atlas_width << std::endl;
    std::cout << "atlas height: " << atlas_height << std::endl;

    std::cout << "loading obj..." << std::endl;


    std::vector<float> t;
    std::map<std::string, std::vector<uint32_t>> tindices;


    load_obj_mat(in_obj_file, t, tindices);

    //print material map
    /*
    for (auto& mat : material_map) {
      std::cout << "material map: " << mat.first << " " << mat.second << std::endl;
    }
    */


    std::cout << "scaling coords..." << std::endl;

    for (const auto& material : tindices) {
      std::string mat = material.first;
      std::cout << "scaling coords: " << mat << std::endl;

      tile tile{0, 0, 0, 0};
      std::cout << "searching for ... " << mat << std::endl;

      if (material_map.find(mat) != material_map.end()) {
        auto texture = material_map[mat];
        if (tile_map.find(texture) != tile_map.end()) {
          tile = tile_map[texture];
          std::cout << mat << " tile: " << tile.x_ << " " << tile.y_ << " " << tile.width_ << " " << tile.height_ << std::endl;
        }
        else { 
          //std::cout << "tile was not found" << std::endl;
        }
      }
      else { std::cout << "material was not found" << std::endl; exit(1); }

      for (const auto& tindex : tindices[mat]) {
        double u = t[2*(tindex-1)];
        double v = t[2*(tindex-1)+1];

        //std::cout << u << std::endl;
        uint32_t image_dim = std::max(atlas_width, atlas_height);

        double scaled_u = ((double)(tile.x_) + (u)*(double)tile.width_) / (double)image_dim;
        double scaled_v = ((double)(tile.y_) + (v)*(double)tile.height_) / (double)image_dim;
        scaled_u = std::min(0.999999999, std::max(0.0000001, scaled_u));
        scaled_v = std::min(0.999999999, std::max(0.0000001, scaled_v));

        //std::cout << scaled_u << std::endl;

        t[2*(tindex-1)] = scaled_u;
        t[2*(tindex-1)+1] = scaled_v;

        //line = "vt " + std::to_string(scaled_u) + " " + std::to_string(scaled_v);

      }
            
      
    }


    std::cout << "writing .obj file..." << std::endl;
    std::ifstream obj_file_in(in_obj_file.c_str());

    std::ofstream obj_file_out;
    obj_file_out.open(out_obj_file.c_str(), std::ios::trunc);

    if (!obj_file_in.is_open()) {
      std::cout << "could not open .obj file" << std::endl;
      exit(-1);
    }

    uint32_t current_t = 0;

    while(std::getline(obj_file_in, line)) {

      if(line.length() >= 2) {
        if (line.substr(0, 6) == "usemtl") {
          current_material = line.substr(7);
          //std::cout << "switch material: " << current_material << std::endl;
          continue;
        }
        else if (line.substr(0, 2) == "vt") {
          line = "vt " + std::to_string(t[current_t++]) + " " + std::to_string(t[current_t++]);
        }
        else {
          line = line.substr(0, line.size()-1);
        }

      }
      obj_file_out << line << std::endl;
    }

    obj_file_in.close();
    obj_file_out.close();


    return 0;

}



