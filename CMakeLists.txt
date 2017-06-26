############################################################
# CMake Build Script

############################################################
# Project settings
cmake_minimum_required(VERSION 2.8.7)
project(lamure)

set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE TYPE INTERNAL FORCE )

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinsizeRel."
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
set(TEST_OUTPUT_PATH ${CMAKE_BINARY_DIR}/test)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(LAMURE_CONFIG_DIR ${CMAKE_BINARY_DIR})

option (LAMURE_USE_CGAL_FOR_NNI "Set to enable CGAL library for natural neighbor interpolation. NNI will not work without CGAL." ON)
option (LAMURE_ENABLE_ALTERNATIVE_COMPUTATION_STRATEGIES "Enables preprocessing strategies different than NDC (requries CGAL)." OFF)

if (LAMURE_ENABLE_ALTERNATIVE_COMPUTATION_STRATEGIES)
add_definitions(-DCMAKE_OPTION_ENABLE_ALTERNATIVE_STRATEGIES)
endif()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "default install path" FORCE )
endif()


file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
file(MAKE_DIRECTORY ${TEST_OUTPUT_PATH})
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_PATH})

############################################################
# compiler flags

if(MSVC)
    set(PROJECT_COMPILE_FLAGS "-D_VARIADIC_MAX=10 -D _SCL_SECURE=0 -D _USE_MATH_DEFINES=1 /MP -D NOMINMAX")
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
################################
SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${GLOBAL_EXT_DIR})

################################
# schism
################################
include(find_schism)

#LINK_DIRECTORIES(${SCHISM_LIBRARY_DIRS})

################################
# Boost
################################
IF (MSVC)
  set(Boost_USE_MULTITHREADED ON)
  set(Boost_USE_STATIC_LIBS ON)
ENDIF ()

IF (MSVC)
  set(BOOST_INCLUDEDIR ${GLOBAL_EXT_DIR}/boost/include)
  set(BOOST_LIBRARYDIR ${GLOBAL_EXT_DIR}/boost/lib)
ELSEIF (UNIX)
  set(BOOST_INCLUDEDIR /opt/boost/boost_1_62_0/include)
  set(BOOST_LIBRARYDIR /opt/boost/boost_1_62_0/lib)
ENDIF ()

find_package( Boost 1.54.0 REQUIRED chrono date_time filesystem program_options regex system thread timer iostreams)
find_package( Threads )

ADD_DEFINITIONS("-DBOOST_ALL_NO_LIB")

#INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
#LINK_DIRECTORIES(${Boost_LIBRARY_DIR})

################################
# GLUT
################################
SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${GLOBAL_EXT_DIR}/freeglut/include)
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${GLOBAL_EXT_DIR}/freeglut/lib)
find_package(GLUT REQUIRED)

#include_directories(${GLUT_INCLUDE_DIR})
#link_directories(${GLUT_LIBRARY_DIRS})

#add_definitions(${GLUT_DEFINITIONS})

################################
# OpenGL
################################
if (UNIX)
  find_package(OpenGL REQUIRED)
  include_directories(${OpenGL_INCLUDE_DIRS})
  link_directories(${OpenGL_LIBRARY_DIRS})
  add_definitions(${OpenGL_DEFINITIONS})
endif ()

################################ 
# FreeImage
################################
include (find_freeimage)

if (${LAMURE_USE_CGAL_FOR_NNI})
  ################################
  # CGAL
  ################################
  include(find_cgal)

  ################################
  # GMP
  ################################
  include(find_gmp)

  ################################
  # MPFR
  ################################
  include(find_mpfr)

else()
  # include NNI alternative here
endif (${LAMURE_USE_CGAL_FOR_NNI})

############################################################
# Build 3rd-party

############################################################
# Targets

add_subdirectory(${CMAKE_SOURCE_DIR}/common)
add_subdirectory(${CMAKE_SOURCE_DIR}/potentially_visible_set_common)
add_subdirectory(${CMAKE_SOURCE_DIR}/preprocessing)
add_subdirectory(${CMAKE_SOURCE_DIR}/rendering)
add_subdirectory(${CMAKE_SOURCE_DIR}/potentially_visible_set_preprocessing)
add_subdirectory(${CMAKE_SOURCE_DIR}/apps)

add_dependencies(lamure_preprocessing lamure_common)
add_dependencies(lamure_rendering lamure_common)

set_target_properties(lamure_common PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(lamure_rendering PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(lamure_preprocessing PROPERTIES DEBUG_POSTFIX "d")