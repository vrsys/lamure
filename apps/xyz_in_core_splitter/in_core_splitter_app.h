// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef IN_CORE_SPLITTER_APP_H_
#define IN_CORE_SPLITTER_APP_H_

#include "xyz_file_formats.h"

#include <map>
#include <iostream>
#include <iomanip>
#include <string>

#define DEFAULT_PRECISION 15

namespace lamure{
namespace app {

class in_core_splitter_app {

public:

	struct bounding_box{
    	xyz pmin;
    	xyz pmax;

      std::ostream& operator<< (std::ostream& o);
  };

	struct point_node{
    	std::vector<xyz*>::iterator begin;
    	std::vector<xyz*>::iterator end;
  };

  //member functions
  std::uint8_t get_longest_side(const bounding_box& b);

  template <typename T>
  inline std::string
  to_string_p(T value, unsigned p);

  bounding_box calculate_bounding_box(const point_node& n);

  int perform_splitting(int argc, char** argv); 

private:

  std::map<std::string, std::pair<xyz_type, std::string> > format_map_ = {
    {".xyz",     {xyz_type::xyz_rgb, ".xyz"}},
    {".xyz_all", {xyz_type::xyz_all, ".xyz_all"}}
  };

}; // in_core_splitter_app


} // namespace lamure app
} // namespace lamure lamure

#endif //IN_CORE_SPLITTER_APP_H