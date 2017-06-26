############################################################
# CMake Build Script for the rendering library

cmake_minimum_required(VERSION 2.8.7)
project(${CMAKE_PROJECT_NAME}_rendering)

set(PROJECT_INCLUDE_DIR RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE PROJECT_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE PROJECT_INCLUDES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_INCLUDE_DIR}/*.h)

file(GLOB_RECURSE PROJECT_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE PROJECT_INCLUDES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_INCLUDE_DIR}/lamure/ren/*.h)

SOURCE_GROUP("rendering" FILES ${PROJECT_SOURCES})
SOURCE_GROUP("rendering" FILES ${PROJECT_INCLUDES})

############################################################
# Shaders

file(GLOB SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.glslv
                  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.glslf)

SOURCE_GROUP("shaders" FILES ${SHADERS})

############################################################
# Build library

include_directories(${PROJECT_INCLUDE_DIR} 
                    ${COMMON_INCLUDE_DIR}
                    ${PVS_COMMON_INCLUDE_DIR} 
                    ${LAMURE_CONFIG_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIR})

link_directories(${SCHISM_LIBRARY_DIRS})

add_library(${PROJECT_NAME} SHARED ${PROJECT_INCLUDES} ${PROJECT_SOURCES} ${SHADERS})

add_dependencies(${PROJECT_NAME} lamure_common lamure_preprocessing lamure_potentially_visible_set_common)

IF (MSVC)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-D LAMURE_RENDERING_LIBRARY")
ENDIF(MSVC)

set(REND_INCLUDE_DIR ${PROJECT_INCLUDE_DIR} PARENT_SCOPE)
set(REND_LIBRARY ${PROJECT_NAME} PARENT_SCOPE)
set(REND_LIBRARY ${PROJECT_NAME})


############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${COMMON_LIBRARY}
    ${PVS_COMMON_LIBRARY}
    optimized ${SCHISM_GL_CORE_LIBRARY} debug ${SCHISM_GL_CORE_LIBRARY_DEBUG}
    optimized ${Boost_THREAD_LIBRARY_RELEASE} debug ${Boost_THREAD_LIBRARY_DEBUG}
    optimized ${Boost_DATE_TIME_LIBRARY_RELEASE} debug ${Boost_DATE_TIME_LIBRARY_DEBUG}
    optimized ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE} debug ${Boost_PROGRAM_OPTIONS_LIBRARY_DEBUG}
    ${FREEIMAGE_LIBRARY}
    )

###############################################################################
# install 
###############################################################################

IF (MSVC)
  install (TARGETS ${PROJECT_NAME}
    CONFIGURATIONS Release
    RUNTIME DESTINATION bin/Release
    LIBRARY DESTINATION lib/Release
    ARCHIVE DESTINATION lib/Release
  )

  install (TARGETS ${PROJECT_NAME}
    CONFIGURATIONS Debug
    RUNTIME DESTINATION bin/Debug
    LIBRARY DESTINATION lib/Debug
    ARCHIVE DESTINATION lib/Debug
  )
ELSEIF (UNIX)
  install (TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION lib
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
ENDIF (MSVC)

# header files 
install(DIRECTORY ${PROJECT_INCLUDE_DIR}/lamure/ren DESTINATION include/lamure FILES_MATCHING PATTERN "*.h")

# shader and material resources
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION share/lamure
        FILES_MATCHING 
        PATTERN "*.glslv"
        PATTERN "*.glslf"
        PATTERN "*.glslg"
        )

# shader and material resources
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/common DESTINATION share/lamure/shaders
        FILES_MATCHING 
        PATTERN "*.glsl"
        PATTERN "*.glsv"
        PATTERN "*.glsf"
        PATTERN "*.glsg"
        )

# shader and material resources
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/common/heatmapping DESTINATION share/lamure/shaders/common
        FILES_MATCHING 
        PATTERN "*.glsl"
        PATTERN "*.glsv"
        PATTERN "*.glsf"
        PATTERN "*.glsg"
        )

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/fonts/Ubuntu.ttf DESTINATION share/lamure/fonts)

