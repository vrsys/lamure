// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_CONFIG_H_
#define LAMURE_CONFIG_H_

#include <string>

namespace lamure {

//#define LAMURE_LOG_TO_FILE 
const std::string LAMURE_LOG_FILE_NAME = "lamure_log.txt";

const std::string LAMURE_GLSL_VERSION = "440 core";
//const std::string LAMURE_GLSL_VERSION = "420 core"; 

const size_t LAMURE_DEFAULT_MEMORY_SIZE = 1024*1024*1024;




} // namespace lamure

#endif


