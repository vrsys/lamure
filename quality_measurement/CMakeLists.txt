############################################################
# CMake Build Script for the quality measurement library

cmake_minimum_required(VERSION 2.8.7)
project(${CMAKE_PROJECT_NAME}_quality_measurement)

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# main sources
file(GLOB MAIN_SRC ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB MAIN_INC ${PROJECT_INCLUDE_DIR}/lamure/qm/*.h)


SOURCE_GROUP("quality_measurement" FILES ${MAIN_SRC})
SOURCE_GROUP("quality_measurement" FILES ${MAIN_INC})


set(PROJECT_SOURCES ${MAIN_SRC})
set(PROJECT_INCLUDES ${MAIN_INC})

include_directories(${PROJECT_INCLUDE_DIR} 
                    ${COMMON_INCLUDE_DIR}
                    ${LAMURE_CONFIG_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIR})

link_directories(${SCHISM_LIBRARY_DIRS})

add_library(${PROJECT_NAME} SHARED ${PROJECT_INCLUDES} ${PROJECT_SOURCES})

add_dependencies(${PROJECT_NAME} lamure_common)

IF (MSVC)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-D LAMURE_QUALITY_MEASUREMENT_LIBRARY")
ENDIF(MSVC)


set(QUALITY_MEASUREMENT_INCLUDE_DIR ${PROJECT_INCLUDE_DIR} PARENT_SCOPE)
set(QUALITY_MEASUREMENT_LIBRARY ${PROJECT_NAME} PARENT_SCOPE)
set(QUALITY_MEASUREMENT_LIBRARY ${PROJECT_NAME})

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${COMMON_LIBRARY}
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
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/lamure/qm DESTINATION include/lamure FILES_MATCHING PATTERN "*.h")
