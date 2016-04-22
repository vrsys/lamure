// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifndef LAMURE_ASSERT_H_
#define LAMURE_ASSERT_H_

#include <lamure/logger.h>
#include <lamure/exception.h>

namespace lamure {

inline static void assert_at(bool condition, const std::string& error, const char* file, int32_t line) {
#if 1
  if (condition) {
    return;
  }
	std::cout << "\n --- ASSERTION FAILED --- \n";
	std::string error_message = std::string("\nfile: ") + std::string(file)
                            + std::string("\nline: ") + lamure::util::to_string(line)
                            + std::string("\nmessage: ") + error;
  LAMURE_LOG_ERROR(error_message << std::string("\n"));
  throw exception_t(error_message);
#endif
}
#define LAMURE_ASSERT(condition, error) (lamure::assert_at(condition, error, __FILE__, __LINE__))
#define ASSERT(condition) (lamure::assert_at(condition, "", __FILE__, __LINE__))

}

#endif
