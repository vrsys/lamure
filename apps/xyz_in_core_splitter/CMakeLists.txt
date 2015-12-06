############################################################
# CMake Build Script for the xyz_in_core_splitter executable

include_directories(${COMMON_INCLUDE_DIR})
include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
					       ${Boost_INCLUDE_DIR})
					

link_directories(${SCHISM_LIBRARY_DIRS})

InitApp(${CMAKE_PROJECT_NAME}_xyz_in_core_splitter)