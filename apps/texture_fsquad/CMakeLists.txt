############################################################
# CMake Build Script for the rendering executable

link_directories(${SCHISM_LIBRARY_DIRS})

add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=8 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )
find_package(ImageMagick COMPONENTS Magick++ MagickWand MagickCore montage)

include_directories(${REND_INCLUDE_DIR}
        ${COMMON_INCLUDE_DIR}
        ${PVS_COMMON_INCLUDE_DIR}
        ${GLUT_INCLUDE_DIR}
        ${GLM_INCLUDE_DIR}
        ${FREEIMAGE_INCLUDE_DIR}
        ${VT_INCLUDE_DIR}
        ${LAMURE_CONFIG_DIR}
        ${ImageMagick_INCLUDE_DIRS})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR})

InitApp(${CMAKE_PROJECT_NAME}_texture_fsquad)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
        ${PROJECT_LIBS}
        ${REND_LIBRARY}
        ${PVS_COMMON_LIBRARY}
        ${VT_LIBRARY}
        ${OpenGL_LIBRARIES}
        ${GLUT_glut_LIBRARY}
        optimized ${SCHISM_CORE_LIBRARY} debug ${SCHISM_CORE_LIBRARY_DEBUG}
        optimized ${SCHISM_GL_CORE_LIBRARY} debug ${SCHISM_GL_CORE_LIBRARY_DEBUG}
        optimized ${SCHISM_GL_UTIL_LIBRARY} debug ${SCHISM_GL_UTIL_LIBRARY_DEBUG}
        optimized ${Boost_REGEX_LIBRARY_RELEASE} debug ${Boost_REGEX_LIBRARY_DEBUG}
        ${ImageMagick_LIBRARIES}
        )

add_dependencies(${PROJECT_NAME} lamure_rendering lamure_common lamure_pvs_common)

MsvcPostBuild(${PROJECT_NAME})

