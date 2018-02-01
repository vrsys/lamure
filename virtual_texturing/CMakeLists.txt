############################################################
# CMake Build Script for the svt library

cmake_minimum_required(VERSION 2.8.7)
project(${CMAKE_PROJECT_NAME}_virtual_texturing)

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=8 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )

find_package(ImageMagick COMPONENTS Magick++ MagickWand MagickCore montage)

# main sources
file(GLOB MAIN_SRC ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB MAIN_INC ${PROJECT_INCLUDE_DIR}/lamure/vt/*.h)

file(GLOB EXT_SRC ${PROJECT_SOURCE_DIR}/ext/*.cpp)
file(GLOB EXT_INC ${PROJECT_INCLUDE_DIR}/lamure/vt/ext/*.h)

file(GLOB PRE_SRC ${PROJECT_SOURCE_DIR}/pre/*.cpp)
file(GLOB PRE_INC ${PROJECT_INCLUDE_DIR}/lamure/vt/pre/*.h)

file(GLOB OOC_SRC ${PROJECT_SOURCE_DIR}/ooc/*.cpp)
file(GLOB OOC_INC ${PROJECT_INCLUDE_DIR}/lamure/vt/ooc/*.h)

file(GLOB REN_SRC ${PROJECT_SOURCE_DIR}/ren/*.cpp)
file(GLOB REN_INC ${PROJECT_INCLUDE_DIR}/lamure/vt/ren/*.h)

SOURCE_GROUP("virtual_texturing" FILES ${MAIN_SRC})
SOURCE_GROUP("virtual_texturing" FILES ${MAIN_INC})

SOURCE_GROUP("ext" FILES ${EXT_SRC})
SOURCE_GROUP("ext" FILES ${EXT_INC})

SOURCE_GROUP("pre" FILES ${PRE_SRC})
SOURCE_GROUP("pre" FILES ${PRE_INC})

SOURCE_GROUP("ooc" FILES ${OOC_SRC})
SOURCE_GROUP("ooc" FILES ${OOC_INC})

SOURCE_GROUP("ren" FILES ${REN_SRC})
SOURCE_GROUP("ren" FILES ${REN_INC})

set(PROJECT_SOURCES ${MAIN_SRC} ${EXT_SRC} ${PRE_SRC} ${REN_SRC} ${OOC_SRC})
set(PROJECT_INCLUDES ${MAIN_INC} ${EXT_INC} ${PRE_INC} ${REN_INC} ${OOC_INC} src/pre/Index.cpp include/lamure/vt/pre/Index.h include/lamure/vt/pre/OffsetIndex.cpp include/lamure/vt/pre/OffsetIndex.h)

include_directories(${PROJECT_INCLUDE_DIR}
        ${COMMON_INCLUDE_DIR}
        ${LAMURE_CONFIG_DIR}
        ${CGAL_INCLUDE_DIR}
        ${GMP_INCLUDE_DIR}
        #${GLFW_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIR}
        ${ImageMagick_INCLUDE_DIRS}
        ${MPFR_INCLUDE_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR})

link_directories(${SCHISM_LIBRARY_DIRS}
        #${GLFW_LIBRARIES}
        ${GLEW_LIBRARY}
        )

add_library(${PROJECT_NAME} SHARED
        ${PROJECT_INCLUDES}
        ${PROJECT_SOURCES}
        #${GLFW_LIBRARIES}
        ${GLEW_LIBRARY})

add_dependencies(${PROJECT_NAME} lamure_common)

IF (MSVC)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-D LAMURE_PREPROCESSING_LIBRARY -DBOOST_ALL_NO_LIB")
ENDIF(MSVC)


set(VT_INCLUDE_DIR ${PROJECT_INCLUDE_DIR} PARENT_SCOPE)
set(VT_LIBRARY ${PROJECT_NAME} PARENT_SCOPE)
set(VT_LIBRARY ${PROJECT_NAME})

############################################################
# Libraries



target_link_libraries(${PROJECT_NAME}
        ${COMMON_LIBRARY}
        ${PROJECT_LIBS}
        ${ImageMagick_LIBRARIES}
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
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/lamure/vt DESTINATION include/lamure FILES_MATCHING PATTERN "*.h")

# shader resources
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION share/lamure
        FILES_MATCHING
        PATTERN "*.glslv"
        PATTERN "*.glslf")

# material and geometry resources
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/primitives DESTINATION share/lamure
        FILES_MATCHING
        PATTERN "*.mtl"
        PATTERN "*.obj")

# material and geometry resources
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/fonts DESTINATION share/lamure
        FILES_MATCHING
        PATTERN "*.ttf")