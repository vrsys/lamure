############################################################
# CMake Build Script for PLY library

IF (UNIX)
project(ply_installer)
cmake_minimum_required(VERSION 2.8.7)

include(ExternalProject)

ExternalProject_Add(
    ply
    GIT_REPOSITORY https://github.com/4og/libply.git
    TIMEOUT 10
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>/inst -DCMAKE_CXX_FLAGS=\"-fPIC\"
    UPDATE_COMMAND ""
    )

ExternalProject_Get_Property(ply source_dir)
ExternalProject_Get_Property(ply install_dir)
ExternalProject_Get_Property(ply binary_dir)

# Set paths
set(PLY_INCLUDE_DIR ${install_dir}/inst/include PARENT_SCOPE)
set(PLY_LIBRARY ${install_dir}/inst/lib/libply.a PARENT_SCOPE)
set(PLY_LIBRARY_DEBUG ${install_dir}/inst/lib/libply.a PARENT_SCOPE)

ELSEIF (MSVC)
  include(find_ply)
ENDIF (UNIX)
