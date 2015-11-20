############################################################
# CMake Build Script for the preprocessing library

cmake_minimum_required(VERSION 2.8.7)
project(${CMAKE_PROJECT_NAME}_preprocessing)

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# main sources
file(GLOB MAIN_SRC ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB MAIN_INC ${PROJECT_INCLUDE_DIR}/lamure/pre/*.h)

# IO
file(GLOB IO_SRC ${PROJECT_SOURCE_DIR}/io/*.cpp)
file(GLOB IO_INC ${PROJECT_INCLUDE_DIR}/lamure/pre/io/*.h)

SOURCE_GROUP("preprocessing" FILES ${MAIN_SRC})
SOURCE_GROUP("preprocessing" FILES ${MAIN_INC})

SOURCE_GROUP("io" FILES ${IO_SRC})
SOURCE_GROUP("io" FILES ${IO_INC})

set(PROJECT_SOURCES ${MAIN_SRC} ${IO_SRC})
set(PROJECT_INCLUDES ${MAIN_INC} ${IO_INC})

include_directories(${PROJECT_INCLUDE_DIR} 
                    ${COMMON_INCLUDE_DIR}  
                    ${PLY_INCLUDE_DIR} 
                    ${LAMURE_CONFIG_DIR})

add_library(${PROJECT_NAME} SHARED ${PROJECT_INCLUDES} ${PROJECT_SOURCES})

add_dependencies(${PROJECT_NAME} lamure_common)

IF (MSVC)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-D LAMURE_PREPROCESSING_LIBRARY")
ENDIF(MSVC)

IF (UNIX)
  add_dependencies(${PROJECT_NAME} ply)
ENDIF (UNIX)

set(PREPROC_INCLUDE_DIR ${PROJECT_INCLUDE_DIR} PARENT_SCOPE)
set(PREPROC_LIBRARY ${PROJECT_NAME} PARENT_SCOPE)
set(PREPROC_LIBRARY ${PROJECT_NAME})

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${COMMON_LIBRARY}
    ${SCHISM_LIBRARIES}
    ${BOOST_LIBRARIES}
    optimized ${PLY_LIBRARY} debug ${PLY_LIBRARY_DEBUG}
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
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/lamure/pre DESTINATION include/lamure FILES_MATCHING PATTERN "*.h")
