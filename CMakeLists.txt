############################################################
# CMake Build Script

############################################################
# Project settings
cmake_minimum_required(VERSION 2.8.7)
project(lamure)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

get_filename_component(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} ABSOLUTE)

message(STATUS "Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

############################################################
# Directories

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
set(THIRD_PARTY_PROJECTS_DIR ${CMAKE_SOURCE_DIR}/third_party)
set(GLOBAL_EXT_DIR ${CMAKE_SOURCE_DIR}/externals)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(LAMURE_CONFIG_DIR ${CMAKE_BINARY_DIR})


file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})

############################################################
# Compiler flags

if(MSVC)
    set(PROJECT_COMPILE_FLAGS "-D_VARIADIC_MAX=10 -D _SCL_SECURE=0 -D _USE_MATH_DEFINES=1 /MP")
    set(PROJECT_LIBS "")
elseif(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(PROJECT_COMPILE_FLAGS "-std=c++1y -Wall -pthread -fPIC -fopenmp")
    set(PROJECT_LIBS "pthread")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROJECT_COMPILE_FLAGS}")

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")

############################################################
# Externals
include(find_compiler)
include(find_schism)

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS})
link_directories(${SCHISM_LIBRARY_DIRS})

include(find_boost)
include_directories(SYSTEM ${BOOST_INCLUDE_DIRS})
link_directories(${BOOST_LIBRARY_DIRS})

include(find_cgal)

# GLUT
if (MSVC)
  include(find_freeglut)
  include(find_hidapi)
  
elseif (UNIX)
  find_package(GLUT REQUIRED)
  include_directories(${GLUT_INCLUDE_DIRS})
  link_directories(${GLUT_LIBRARY_DIRS})
  add_definitions(${GLUT_DEFINITIONS})
  set(GLUT_LIBRARY_DEBUG ${GLUT_LIBRARY})
endif (MSVC)

# OpenGL
if (MSVC)
elseif (UNIX)
  find_package(OpenGL REQUIRED)
  include_directories(${OpenGL_INCLUDE_DIRS})
  link_directories(${OpenGL_LIBRARY_DIRS})
  add_definitions(${OpenGL_DEFINITIONS})
endif (MSVC)

#list(APPEND PROJECT_LIBS ${MALLOC_LIBRARY})
#list(APPEND PROJECT_LIBS ${PROFILER_LIBRARY})

include (find_freeimage)

############################################################
# Build 3rd-party

add_subdirectory(${THIRD_PARTY_PROJECTS_DIR}/ply)

############################################################
# Targets

add_subdirectory(${CMAKE_SOURCE_DIR}/common)
add_subdirectory(${CMAKE_SOURCE_DIR}/preprocessing)
add_subdirectory(${CMAKE_SOURCE_DIR}/rendering)
add_subdirectory(${CMAKE_SOURCE_DIR}/apps)

add_dependencies(lamure_preprocessing lamure_common)
add_dependencies(lamure_rendering lamure_common)

