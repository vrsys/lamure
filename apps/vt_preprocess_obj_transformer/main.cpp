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
#include "preprocess_obj_transformer.h"

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
            !cmd_option_exists(argv, argv+argc, "-atlas")) {
      std::cout << "Usage: " << argv[0]
                << " -obj <input_mesh.obj> -atlas <preprocessed_atlas.atlas>"
                << std::endl
                <<"INFO: argv[0] " << std::endl ;
      return 0;
    }

    std::string in_obj_file   = get_cmd_option(argv, argv+argc, "-obj");
    std::string in_atlas_file = get_cmd_option(argv, argv+argc, "-atlas");

    auto *obj_transformer = new preprocess_obj_transformer(in_obj_file, in_atlas_file);

    obj_transformer->scale();

    return 0;
}



