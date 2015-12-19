############################################################
# CMake Build Script for applications

cmake_minimum_required(VERSION 2.8.7)
project(${CMAKE_PROJECT_NAME}_tests)

include("test.cmake")

############################################################
# Add subdirectories

file(GLOB FILE_PATHS ${PROJECT_SOURCE_DIR}/*/CMakeLists.txt)

foreach(FILE_PATH ${FILE_PATHS})
    get_filename_component(DIR ${FILE_PATH} PATH)
    add_subdirectory(${DIR})
endforeach()

