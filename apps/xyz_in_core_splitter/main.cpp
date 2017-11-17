// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include "in_core_splitter_app.h"
// numberofpoints_in_input maxpoints_in_each_output input.xyz output_prefix
int main(int argc, char** argv){

  lamure::app::in_core_splitter_app splitter_app;

  splitter_app.perform_splitting(argc, argv);
  
  return 0;

}
