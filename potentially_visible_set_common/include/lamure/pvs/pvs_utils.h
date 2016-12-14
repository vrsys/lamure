// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_PVS_PVSUTILS_H_
#define LAMURE_PVS_PVSUTILS_H_

#include <string>

#include "lamure/pvs/grid.h"

namespace lamure
{
namespace pvs
{

void analyze_grid_visibility(const grid* input_grid, const unsigned int& num_steps, const std::string& output_file_name);

}
}

#endif