#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <limits>

#include <boost/algorithm/string.hpp>


#include <endian.h>

#ifndef UTILSH
#define UTILSH



struct Utils
{

	static std::vector<int> load_tex_paths_from_mtl(const std::string& mtl_filename, 
										std::vector<std::string>& texture_paths,
										std::vector<scm::math::vec2i>& texture_dimensions) {

	    std::vector<int> missing_textures;

	    //parse .mtl file
	    std::cout << "loading .mtl file ..." << std::endl;
	    std::ifstream mtl_file(mtl_filename.c_str());
	    if (!mtl_file.is_open()) {
	      std::cout << "ERROR: could not open .mtl file" << std::endl;
	      exit(EXIT_FAILURE);
	    }

	    //get path from mtl filename
	    const std::string dir_path = mtl_filename.substr(0, mtl_filename.find_last_of("/\\") + 1);


	    std::string line;
	    while (std::getline(mtl_file, line)) {
	      boost::trim(line);
	      if(line.length() >= 2) {
	        if (line[0] == '#') {
	          continue;
	        }
	        else if (line.substr(0, 6) == "map_Kd") {
	          //std::cout << "Parsing map_Kd" << "\n";
	          //std::cout << "LION: " << line << "\n";
	          std::string current_texture = line.substr(7);
	          current_texture.erase(std::remove(current_texture.begin(), current_texture.end(), '\n'), current_texture.end());
	          current_texture.erase(std::remove(current_texture.begin(), current_texture.end(), '\r'), current_texture.end());
	          boost::trim(current_texture);
	          
	          //replace jpg file types with png file types
	          //in pipeline script, these are converted before this point
	          current_texture = current_texture.substr(0,current_texture.length()-4).append(".png");

	          std::string full_path = dir_path + current_texture;

	          //check if file exists and is a png
	          bool file_good = true;
		      if(!boost::filesystem::exists(full_path)) {file_good = false;}
		      if(!boost::algorithm::ends_with(full_path, ".png")) {file_good = false;}

		      if (!file_good)
		      {
		        std::cout << "WARNING: texture does not exist or is not a PNG (" << full_path << ")\n";
		        texture_dimensions.push_back(scm::math::vec2i(-1,-1));
		        missing_textures.push_back(texture_dimensions.size() - 1);
		      }
		      else {
		        //get dimensions of file
		        texture_dimensions.push_back(Utils::get_png_dimensions(full_path));
		        std::cout << "Texture: (" << texture_dimensions.back().x << "x" << texture_dimensions.back().y << ") " << full_path << std::endl;
		      }

	          texture_paths.push_back(full_path);
	        }
	      }
	    
	    }

	    mtl_file.close();

	    std::cout << "Parsed texture paths: " << std::endl;

	    for(auto const& texture_path : texture_paths) {
	    	std::cout << texture_path << std::endl;
	    }

	    return missing_textures;

	}

	//reads dimensions of png from header 
	static scm::math::vec2i get_png_dimensions(std::string filepath) { 
	    std::ifstream in(filepath);
	    uint32_t width, height;

	    in.seekg(16);
	    in.read((char *)&width, 4);
	    in.read((char *)&height, 4);

	    width = be32toh(width);
	    height = be32toh(height);

	    return scm::math::vec2i(width, height);
	}


};

#endif