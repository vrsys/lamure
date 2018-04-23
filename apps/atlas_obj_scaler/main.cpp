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
          material_map[current_material] = "";
          std::cout << "switch material: " << current_material << std::endl;
        }
        else if (line.substr(0, 6) == "map_Kd") {
          std::string current_texture = line.substr(7);
          std::cout << current_material << std::endl;
          std::cout << current_texture << std::endl;
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

    std::cout << "scaling .obj file..." << std::endl;
    std::ifstream obj_file_in(in_obj_file.c_str());

    std::ofstream obj_file_out;
    obj_file_out.open(out_obj_file.c_str(), std::ios::trunc);


    if (!obj_file_in.is_open()) {
      std::cout << "could not open .obj file" << std::endl;
      exit(-1);
    }

    while(std::getline(obj_file_in, line)) {

      if(line.length() >= 2) {
        if (line.substr(0, 6) == "usemtl") {
          current_material = line.substr(7);
          std::cout << "switch material: " << current_material << std::endl;
        }
        else if (line.substr(0, 2) == "vt") {

          std::vector<std::string> values;
          split(line.begin(), line.end(), std::back_inserter(values), ' ');
          double u = atof(values[1].c_str());
          double v = atoi(values[2].c_str());
          
          if (material_map.find(current_material) != material_map.end()) {
            auto texture = material_map[current_material];
            if (tile_map.find(texture) != tile_map.end()) {
              auto tile = tile_map[texture];
              
              double scaled_u = (tile.x_ / (double)atlas_width) + (u*tile.width_)/atlas_width;
              double scaled_v = (tile.y_ / (double)atlas_height) + (v*tile.height_)/atlas_height;

              line = "vt " + std::to_string(scaled_u) + " " + std::to_string(scaled_v);

            }
            else { 
              //std::cout << "tile was not found" << std::endl;
            }
          }
          else { std::cout << "material was not found" << std::endl; exit(1); }
          
        }

      }
      obj_file_out << line << std::endl;
    }

    obj_file_in.close();
    obj_file_out.close();

    

    return 0;

}



