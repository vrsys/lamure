############################################################
# CMake Build Script for the rendering executable

include_directories(${REND_INCLUDE_DIR}
        ${COMMON_INCLUDE_DIR}
        ${FREEIMAGE_INCLUDE_DIR})

include_directories(SYSTEM ${Boost_INCLUDE_DIR})

InitApp(${CMAKE_PROJECT_NAME}_stitch_it)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

file(GLOB PROJECT_INCLUDES ${PROJECT_SOURCE_DIR}/*.h)
file(GLOB PROJECT_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
        ${PROJECT_LIBS}
        ${REND_LIBRARY}
        optimized ${Boost_REGEX_LIBRARY_RELEASE} debug ${Boost_REGEX_LIBRARY_DEBUG}
        stdc++fs # C++17
        )

add_dependencies(${PROJECT_NAME} lamure_rendering lamure_common lamure_pvs_common)

MsvcPostBuild(${PROJECT_NAME})

