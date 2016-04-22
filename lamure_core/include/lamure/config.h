
#ifndef LAMURE_CONFIG_H_
#define LAMURE_CONFIG_H_

#include <string>

namespace lamure {

#define LAMURE_LOG_TO_FILE 
const std::string LAMURE_LOG_FILE_NAME = "last_log.txt";

const std::string LAMURE_GLSL_VERSION = "440";
//const std::string LAMURE_GLSL_VERSION = "410"; 

const size_t LAMURE_DEFAULT_MEMORY_SIZE = 1024*1024*1024;




} // namespace lamure

#endif


