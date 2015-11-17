// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

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
#define DEFAULT_PRECISION 15

namespace{

  struct xyzrgb{
    float x;
    float y;
    float z;
    unsigned char r;
    unsigned char g;
    unsigned char b;
  };

#if 0
  out << std::setprecision(DEFAULT_PRECISION) <<
#endif

  struct bbx{
    xyzrgb pmin;
    xyzrgb pmax;
  };
  
  std::ostream& operator<< (std::ostream& o, const bbx& b){
    o << "(" << b.pmin.x << ","
      << b.pmin.y << ","
      << b.pmin.z << ") -> ("
      << b.pmax.x << ","
      << b.pmax.y << ","
      << b.pmax.z << ")";
    return o;
  }

  struct node{
    std::vector<xyzrgb>::iterator begin;
    std::vector<xyzrgb>::iterator end;
  };

  unsigned max_length(const bbx& b){
    float x(b.pmax.x - b.pmin.x);
    float y(b.pmax.y - b.pmin.y);
    float z(b.pmax.z - b.pmin.z);
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

  bbx calc_bbx(const node& n){
    bbx b;
    b.pmin.x = std::numeric_limits<float>::max();
    b.pmin.y = std::numeric_limits<float>::max();
    b.pmin.z = std::numeric_limits<float>::max();

    b.pmax.x = std::numeric_limits<float>::lowest();
    b.pmax.y = std::numeric_limits<float>::lowest();
    b.pmax.z = std::numeric_limits<float>::lowest();
    for(auto i = n.begin; i != n.end; ++i){
      b.pmin.x = std::min(b.pmin.x,i->x);
      b.pmin.y = std::min(b.pmin.y,i->y);
      b.pmin.z = std::min(b.pmin.z,i->z);

      b.pmax.x = std::max(b.pmax.x,i->x);
      b.pmax.y = std::max(b.pmax.y,i->y);
      b.pmax.z = std::max(b.pmax.z,i->z);
    }
    return b;
  }



  template <class T>
  inline std::string
  toStringP(T value, unsigned p)
  {
    std::ostringstream stream;
    stream << std::setw(p) << std::setfill('0') << value;
    return stream.str();
  }



}


// numberofpoints_in_input maxpoints_in_each_output input.xyz output_prefix
int main(int argc, char** argv){
  if(argc != 5){
  	std::cout << "usage: " << argv[0]
		  << " numberofpoints_in_input maxpoints_in_each_output input.xyz output_prefix" << std::endl;
    return 0;
  }

  // allocate vector of correct size
  size_t num_points(atoll(argv[1]));
  size_t max_points(atoll(argv[2]));
  std::cout << "allocating memory for " << num_points << " points" << std::endl;
  std::vector<xyzrgb> pc(num_points);
  // parse input
  size_t num_points_loaded(0);
  std::ifstream input;
  input.open(argv[3]);
  for(auto& p : pc){
    input >> p.x;
    input >> p.y;
    input >> p.z;
    unsigned tmp;
    input >> tmp;
    p.r = tmp;
    input >> tmp;
    p.g = tmp;
    input >> tmp;
    p.b = tmp;
    ++num_points_loaded;

    //std::cout << p.x << " " << p.y << " " << p.z << " " << (unsigned) p.r << " " << (unsigned) p.g << " " << (unsigned) p.g << std::endl;

  }
  input.close();
  std::cout << "loaded " << num_points_loaded << " points." << std::endl;


  unsigned partnum(0);
  node root_node;
  root_node.begin = pc.begin();
  root_node.end = pc.end();

  
  std::stack<node> remaining;
  remaining.push(root_node);
  while(!remaining.empty()){
    node current_node = remaining.top();
    remaining.pop();
    const size_t points_contained(current_node.end - current_node.begin);
    if(points_contained > max_points){ // SPLIT NODE
      auto b = calc_bbx(current_node);
      unsigned split_axis(max_length(b));
      switch(split_axis){
      case 0:
	// sort along x axis
	std::sort(current_node.begin, current_node.end, 
		  [](const xyzrgb& a, const xyzrgb& b) -> bool
		  { 
		    return a.x < b.x;
		  });
	break;
      case 1:
	// sort along y axis
	std::sort(current_node.begin, current_node.end, 
		  [](const xyzrgb& a, const xyzrgb& b) -> bool
		  { 
		    return a.y < b.y;
		  });
	break;
      case 2:
	// sort along z axis
	std::sort(current_node.begin, current_node.end, 
		  [](const xyzrgb& a, const xyzrgb& b) -> bool
		  { 
		    return a.z < b.z;
		  });
	break;
      }
      const size_t split = points_contained/2;
      node left_child;
      left_child.begin  = current_node.begin;
      left_child.end    = current_node.begin + split;
      node right_child;
      right_child.begin = current_node.begin + split;
      right_child.end   = current_node.end;
      remaining.push(left_child);
      remaining.push(right_child);
    }
    else if(0){  // WRITE NODE TO FILE
      std::ofstream output;
      std::string outfilename(argv[4] + toStringP(++partnum, 5) + ".bin");
      std::cout << "writing " << outfilename << " of size " << points_contained << std::endl;
      output.open(outfilename.c_str(), std::ofstream::binary);
    
      for(auto i = current_node.begin; i != current_node.end; ++i){
	output << i->x 
	       << i->y 
	       << i->z
	       << i->r
	       << i->g
	       << i->b;
      }
      
      output.close();
    }
    else{
      std::ofstream output;
      std::string outfilename(argv[4] + toStringP(++partnum, 5) + ".xyz");
      std::cout << "writing " << outfilename << " of size " << points_contained << std::endl;
      output.open(outfilename.c_str());
      for(auto i = current_node.begin; i != current_node.end; ++i){
	output << std::setprecision(DEFAULT_PRECISION) << i->x << " "
	       << std::setprecision(DEFAULT_PRECISION) << i->y << " "
	       << std::setprecision(DEFAULT_PRECISION) << i->z << " "
	       << (unsigned) i->r << " "
	       << (unsigned) i->g << " "
	       << (unsigned) i->b << std::endl;
      }
      output.close();
    }
  }
  

  return 0;
}
