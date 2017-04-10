############################################################
# CMake Build Script for the mesh library

cmake_minimum_required(VERSION 2.8.7)
project(${CMAKE_PROJECT_NAME}_mesh)

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# main sources
file(GLOB_RECURSE PROJECT_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE PROJECT_INCLUDES ${PROJECT_INCLUDE_DIR}/*.h)
file(GLOB_RECURSE PROJECT_INLINES ${PROJECT_INCLUDE_DIR}/*.inl)

SOURCE_GROUP("mesh" FILES ${PROJECT_SOURCES})
SOURCE_GROUP("mesh" FILES ${PROJECT_INLCUDES})
SOURCE_GROUP("mesh" FILES ${PROJECT_INLINES})

include_directories(${PROJECT_INCLUDE_DIR} 
                    ${LAMURE_CORE_INCLUDE_DIR}
                    ${LAMURE_CONFIG_DIR}
                    ${CGAL_INCLUDE_DIR}
                    ${GMP_INCLUDE_DIR} 
                    ${MPFR_INCLUDE_DIR}
                    )

include_directories(SYSTEM ${Boost_INCLUDE_DIR})

add_library(${PROJECT_NAME} SHARED ${PROJECT_INCLUDES} ${PROJECT_INLINES} ${PROJECT_SOURCES})

add_dependencies(${PROJECT_NAME} lamure_core)

IF (MSVC)
        SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-D LAMURE_MESH_LIBRARY -DBOOST_ALL_NO_LIB")
ENDIF(MSVC)


set(LAMURE_MESH_INCLUDE_DIR ${PROJECT_INCLUDE_DIR} PARENT_SCOPE)
set(LAMURE_MESH_LIBRARY ${PROJECT_NAME} PARENT_SCOPE)
set(LAMURE_MESH_LIBRARY ${PROJECT_NAME})

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${LAMURE_CORE_LIBRARY}
    ${PROJECT_LIBS}
    ${GMP_LIBRARY}
    ${MPFR_LIBRARY}
    optimized ${Boost_THREAD_LIBRARY_RELEASE} debug ${Boost_THREAD_LIBRARY_DEBUG}
    optimized ${CGAL_LIBRARY} debug ${CGAL_LIBRARY_DEBUG}
    optimized ${CGAL_CORE_LIBRARY} debug ${CGAL_CORE_LIBRARY_DEBUG}
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
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/lamure/mesh DESTINATION include/lamure FILES_MATCHING PATTERN "*.h")
