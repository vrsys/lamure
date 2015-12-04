// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "xyz_file_formats.h"
#include "in_core_splitter_app.h"

#include <cmath>
#include <string>
#include <algorithm>
#include <vector>
#include <stack>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <limits>
#include <iomanip>

namespace lamure {
namespace app {

std::ostream& in_core_splitter_app::bounding_box::
operator<< (std::ostream& o) {
    o << "(" << this->pmin.x << ","
             << this->pmin.y << ","
             << this->pmin.z << ") -> ("
             << this->pmax.x << ","
             << this->pmax.y << ","
             << this->pmax.z << ")";
    return o;
}

std::uint8_t in_core_splitter_app::
get_longest_side(const bounding_box& b) {
    real x(b.pmax.x - b.pmin.x);
    real y(b.pmax.y - b.pmin.y);
    real z(b.pmax.z - b.pmin.z);
    if(x > y){
      if(x > z){
	return 0;
      }
      else{
	return 2;
      }
    }
    else{
      if(y > z){
	return 1;
      }
      else{
	return 2;
      }
    }
}


template <typename T>
inline std::string
in_core_splitter_app::to_string_p(T value, unsigned p) {
    std::ostringstream stream;
    stream << std::setw(p) << std::setfill('0') << value;
    return stream.str();
}

in_core_splitter_app::bounding_box in_core_splitter_app::
calculate_bounding_box(const point_node& n) {
	bounding_box b;
    b.pmin.x = std::numeric_limits<float>::max();
    b.pmin.y = std::numeric_limits<float>::max();
    b.pmin.z = std::numeric_limits<float>::max();

    b.pmax.x = std::numeric_limits<float>::lowest();
    b.pmax.y = std::numeric_limits<float>::lowest();
    b.pmax.z = std::numeric_limits<float>::lowest();
    for(auto i = n.begin; i != n.end; ++i) {
      b.pmin.x = std::min(b.pmin.x,(*i)->x);
      b.pmin.y = std::min(b.pmin.y,(*i)->y);
      b.pmin.z = std::min(b.pmin.z,(*i)->z);

      b.pmax.x = std::max(b.pmax.x,(*i)->x);
      b.pmax.y = std::max(b.pmax.y,(*i)->y);
      b.pmax.z = std::max(b.pmax.z,(*i)->z);
    }
    return b;
}

int in_core_splitter_app::
perform_splitting(int argc, char** argv) {
  if(argc != 5){
  	std::cout << "usage: " << argv[0]
		  << " numberofpoints_in_input maxpoints_in_each_output input.xyz output_prefix" << std::endl;
    return 0;
  }



  std::string const in_filename = argv[3];
  std::string const file_extension = in_filename.substr(in_filename.find_last_of("."));

  std::cout << "File extension: " << file_extension << "\n";

  if( format_map_.find(file_extension) == format_map_.end() ) {
    std::cout << "Unknown file extension. Please use one of the following file-types:\n";

    for( auto const& current_type : format_map_ ) {
      std::cout << current_type.first << "\n";
    }

    std::cout <<"Aborting.\n\n";
    return -1;
  }

  auto const& in_out_type = format_map_[file_extension];

  // allocate vector of correct size
  size_t num_points(atoll(argv[1]));
  size_t max_points(atoll(argv[2]));
  std::cout << "allocating memory for " << num_points << " points" << std::endl;
  std::vector<xyz*> pc(num_points);
  //actual allocation
  for(auto& p : pc) {
    switch(in_out_type.first) {
      case (xyz_type::xyz_rgb):
        p = new xyz_rgb;
        break;
      case (xyz_type::xyz_all):
        p = new xyz_all;
        break;
      default:
        std::cout << "no valid file extension. aborting. \n\n";
        return -1;
        break;
    }
  }

  // parse input
  size_t num_points_loaded(0);
  std::ifstream input;
  input.open(in_filename);

  for(auto& p : pc){
    p->read_from_file(input);
    ++num_points_loaded;
  }

  input.close();
  std::cout << "loaded " << num_points_loaded << " points." << std::endl;


  unsigned partnum(0);
  point_node root_node;
  root_node.begin = pc.begin();
  root_node.end = pc.end();

  
  std::stack<point_node> remaining;
  remaining.push(root_node);
  while(!remaining.empty()){
    point_node current_node = remaining.top();
    remaining.pop();
    const size_t points_contained(current_node.end - current_node.begin);
    if(points_contained > max_points){ // SPLIT NODE
      auto b = calculate_bounding_box(current_node);
      unsigned split_axis(get_longest_side(b));
      switch(split_axis){
      case 0:
	// sort along x axis
	std::sort(current_node.begin, current_node.end, 
		  []( xyz* const a, xyz* const b) -> bool
		  { 
		    return a->x < b->x;
		  });
	break;
      case 1:
	// sort along y axis
	std::sort(current_node.begin, current_node.end, 
		  [](xyz* const a, xyz* const b) -> bool
		  { 
		    return a->y < b->y;
		  });
	break;
      case 2:
	// sort along z axis
	std::sort(current_node.begin, current_node.end, 
		  [](xyz* const a, xyz* const b) -> bool
		  { 
		    return a->z < b->z;
		  });
	break;
      }
      const size_t split = points_contained/2;
      point_node left_child;
      left_child.begin  = current_node.begin;
      left_child.end    = current_node.begin + split;
      point_node right_child;
      right_child.begin = current_node.begin + split;
      right_child.end   = current_node.end;
      remaining.push(left_child);
      remaining.push(right_child);
    }
    else if(0){  // WRITE NODE TO FILE
      std::ofstream output;
      std::string outfilename(argv[4] + to_string_p(++partnum, 5) + ".bin");
      std::cout << "writing " << outfilename << " of size " << points_contained << std::endl;
      output.open(outfilename.c_str(), std::ofstream::binary);
    
      for(auto point_it = current_node.begin; point_it != current_node.end; ++point_it){
          (*point_it)->write_to_file(output);
      }
      
      output.close();
    }
    else{
      std::ofstream output;
      std::string outfilename(argv[4] + to_string_p(++partnum, 5) + in_out_type.second);
      std::cout << "writing " << outfilename << " of size " << points_contained << std::endl;
      output.open(outfilename.c_str());
      for(auto point_it = current_node.begin; point_it != current_node.end; ++point_it) {
          (*point_it)->write_to_file(output);
      }
      output.close();
    }
  }
  

  //deallocation
  for(auto& p : pc) {
    delete p;
  }

  return 0;
}


}
}