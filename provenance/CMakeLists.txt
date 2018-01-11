############################################################
#CMake Build Script for the provenance library

cmake_minimum_required(VERSION 2.8.7)
project(${CMAKE_PROJECT_NAME}_provenance)

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# main sources
file(GLOB MAIN_SRC ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB MAIN_INC ${PROJECT_INCLUDE_DIR}/lamure/prov/*.h)

# 3dparty
file(GLOB 3DPARTY_SRC ${PROJECT_SOURCE_DIR}/3rd_party/*.cpp)
file(GLOB 3DPARTY_INC ${PROJECT_INCLUDE_DIR}/lamure/prov/3rd_party/*.h)


SOURCE_GROUP("provenance" FILES ${MAIN_SRC})
SOURCE_GROUP("provenance" FILES ${MAIN_INC})

SOURCE_GROUP("3dparty" FILES ${3DPARTY_SRC})
SOURCE_GROUP("3dparty" FILES ${3DPARTY_INC})

set(PROJECT_SOURCES ${MAIN_SRC} ${3DPARTY_SRC})
set(PROJECT_INCLUDES ${MAIN_INC} ${3DPARTY_INC})

include_directories(${PROJECT_INCLUDE_DIR}
                    ${COMMON_INCLUDE_DIR}
                    ${LAMURE_CONFIG_DIR}
                    ${CGAL_INCLUDE_DIR}
                    ${GMP_INCLUDE_DIR} 
                    ${MPFR_INCLUDE_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIR})

link_directories(${SCHISM_LIBRARY_DIRS})

add_library(${PROJECT_NAME} SHARED ${PROJECT_INCLUDES} ${PROJECT_SOURCES})

#add_dependencies(${PROJECT_NAME} lamure_common)

IF (MSVC)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-D LAMURE_PROVENANCE_LIBRARY -DBOOST_ALL_NO_LIB")
ENDIF(MSVC)


set(PROV_INCLUDE_DIR ${PROJECT_INCLUDE_DIR} PARENT_SCOPE)
set(PROV_LIBRARY ${PROJECT_NAME} PARENT_SCOPE)
set(PROV_LIBRARY ${PROJECT_NAME})

############################################################
# Libraries



target_link_libraries(${PROJECT_NAME}
    ${COMMON_LIBRARY}
    ${PROJECT_LIBS}
    optimized ${Boost_SERIALIZATION_LIBRARY_RELEASE} debug ${Boost_SERIALIZATION_LIBRARY_DEBUG}
    optimized ${Boost_THREAD_LIBRARY_RELEASE} debug ${Boost_THREAD_LIBRARY_DEBUG})

if (${LAMURE_USE_CGAL_FOR_NNI})
  target_link_libraries(${PROJECT_NAME}
      ${GMP_LIBRARY}
      ${MPFR_LIBRARY}
      optimized ${CGAL_LIBRARY} debug ${CGAL_LIBRARY_DEBUG}
      optimized ${CGAL_CORE_LIBRARY} debug ${CGAL_CORE_LIBRARY_DEBUG})
endif ()

###############################################################################
# install 
###############################################################################

IF (MSVC)
  install (TARGETS ${PROJECT_NAME}
    CONFIGURATIONS Release
    RUNTIME DESTINATION bin/Release
    LIBRARY DESTINATION lib/Release
    ARCHIVE DESTINATION lib/Release)

  install (TARGETS ${PROJECT_NAME}
    CONFIGURATIONS Debug
    RUNTIME DESTINATION bin/Debug
    LIBRARY DESTINATION lib/Debug
    ARCHIVE DESTINATION lib/Debug)
ELSEIF (UNIX)
  install (TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION lib
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
ENDIF (MSVC)

# header files 
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/lamure/prov DESTINATION include/lamure FILES_MATCHING PATTERN "*.h")
