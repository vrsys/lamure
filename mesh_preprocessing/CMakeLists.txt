############################################################
# CMake Build Script for the mesh preprocessing library

cmake_minimum_required(VERSION 2.8.7)
project(${CMAKE_PROJECT_NAME}_mesh_preprocessing)

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# main sources
file(GLOB MESH_SRC ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/old/*.cpp)
file(GLOB MESH_INC ${PROJECT_INCLUDE_DIR}/lamure/mesh/*.h ${PROJECT_INCLUDE_DIR}/lamure/mesh/old/*.h)
file(GLOB MESH_INL ${PROJECT_INCLUDE_DIR}/lamure/mesh/*.inl)

SOURCE_GROUP("mesh_preprocessing" FILES ${MAIN_SRC})
SOURCE_GROUP("mesh_preprocessing" FILES ${MAIN_INC})
SOURCE_GROUP("mesh_preprocessing" FILES ${MAIN_INL})

set(PROJECT_SOURCES ${MESH_SRC})
set(PROJECT_INCLUDES ${MESH_INC} ${MESH_INL})

find_package(OpenMP)
if(NOT TARGET OpenMP::OpenMP_CXX)
    find_package(Threads REQUIRED)
    add_library(OpenMP::OpenMP_CXX IMPORTED INTERFACE)
    set_property(TARGET OpenMP::OpenMP_CXX PROPERTY INTERFACE_COMPILE_OPTIONS ${OpenMP_CXX_FLAGS})
    # Only works if the same flag is passed to the linker; use CMake 3.9+ otherwise (Intel, AppleClang)
    set_property(TARGET OpenMP::OpenMP_CXX PROPERTY INTERFACE_LINK_LIBRARIES ${OpenMP_CXX_FLAGS} Threads::Threads)
endif()

include_directories(${PROJECT_INCLUDE_DIR}
        ${COMMON_INCLUDE_DIR}
        ${REND_INCLUDE_DIR}
        ${LAMURE_CONFIG_DIR}
        ${GMP_INCLUDE_DIR}
        ${MPFR_INCLUDE_DIR}
        ${CGAL_INCLUDE_DIR}
        )

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR})

link_directories(${SCHISM_LIBRARY_DIRS})

add_library(${PROJECT_NAME} SHARED ${PROJECT_INCLUDES} ${PROJECT_SOURCES})

add_dependencies(${PROJECT_NAME} lamure_common lamure_rendering)

IF (MSVC)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-D LAMURE_MESH_PREPROCESSING_LIBRARY -DBOOST_ALL_NO_LIB")
ENDIF (MSVC)


set(MESH_PREPROC_INCLUDE_DIR ${PROJECT_INCLUDE_DIR} PARENT_SCOPE)
set(MESH_PREPROC_LIBRARY ${PROJECT_NAME} PARENT_SCOPE)
set(MESH_PREPROC_LIBRARY ${PROJECT_NAME})

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
        ${COMMON_LIBRARY}
        ${REND_LIBRARY}
        ${PROJECT_LIBS}
        optimized ${Boost_THREAD_LIBRARY_RELEASE} debug ${Boost_THREAD_LIBRARY_DEBUG}
        optimized ${CGAL_LIBRARY} debug ${CGAL_LIBRARY_DEBUG}
        optimized ${CGAL_CORE_LIBRARY} debug ${CGAL_CORE_LIBRARY_DEBUG}
        )

if (${LAMURE_USE_CGAL_FOR_NNI})
    target_link_libraries(${PROJECT_NAME}
            ${GMP_LIBRARY}
            ${MPFR_LIBRARY}
            )
endif ()

###############################################################################
# install 
###############################################################################

IF (MSVC)
    install(TARGETS ${PROJECT_NAME}
            CONFIGURATIONS Release
            RUNTIME DESTINATION bin/Release
            LIBRARY DESTINATION lib/Release
            ARCHIVE DESTINATION lib/Release
            )

    install(TARGETS ${PROJECT_NAME}
            CONFIGURATIONS Debug
            RUNTIME DESTINATION bin/Debug
            LIBRARY DESTINATION lib/Debug
            ARCHIVE DESTINATION lib/Debug
            )
ELSEIF (UNIX)
    install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION lib
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            )
ENDIF (MSVC)

# header files 
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/lamure/mesh DESTINATION include/lamure FILES_MATCHING PATTERN "*.h")
# inline files 
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/lamure/mesh DESTINATION include/lamure FILES_MATCHING PATTERN "*.inl")


