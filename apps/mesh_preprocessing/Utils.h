#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>

#include <boost/algorithm/string.hpp>

#ifndef UTILSH
#define UTILSH



struct Utils
{

	static bool load_tex_paths_from_mtl(const std::string& mtl_filename, std::vector<std::string>& texture_paths) {

	    //parse .mtl file
	    std::cout << "loading .mtl file ..." << std::endl;
	    std::ifstream mtl_file(mtl_filename.c_str());
	    if (!mtl_file.is_open()) {
	      std::cout << "could not open .mtl file" << std::endl;
	      return false;
	    }

	    //get path from mtl filename
	    const std::string dir_path = mtl_filename.substr(0, mtl_filename.find_last_of("/\\") + 1);

	    std::string line;
	    while (std::getline(mtl_file, line)) {
	      if(line.length() >= 2) {
	        if (line[0] == '#') {
	          continue;
	        }
	        else if (line.substr(0, 6) == "map_Kd") {

	          std::string current_texture = line.substr(7);
	          current_texture.erase(std::remove(current_texture.begin(), current_texture.end(), '\n'), current_texture.end());
	          boost::trim(current_texture);
	          
	          std::string full_path = dir_path + current_texture;

	          //check existence of image file
	          std::ifstream file(full_path);
			  if(!file.good()){
			    std::cout << "Texture specified in .mtl file was not found (" << full_path << ")\n";
			    continue;
			  }

	          texture_paths.push_back(full_path);
	        }
	      }
	    
	    }

	    mtl_file.close();

	    return true;

	}


};

#endif