############################################################
# CMake Build Script for the bvh_extractor executable

include_directories(
        ${VT_INCLUDE_DIR}
        )

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR})


InitApp(${CMAKE_PROJECT_NAME}_vt_preprocessing)

############################################################
# Libraries
target_link_libraries(${PROJECT_NAME}
        ${PROJECT_LIBS}
        ${REND_LIBRARY}
        ${VT_LIBRARY}
        ${OpenGL_LIBRARY}
        ${GLFW_LIBRARIES}
        ${GLEW_LIBRARY}
        ${OPENGL_LIBRARY}
        optimized ${SCHISM_CORE_LIBRARY} debug ${SCHISM_CORE_LIBRARY_DEBUG}
        optimized ${SCHISM_GL_CORE_LIBRARY} debug ${SCHISM_GL_CORE_LIBRARY_DEBUG}
        optimized ${SCHISM_GL_UTIL_LIBRARY} debug ${SCHISM_GL_UTIL_LIBRARY_DEBUG}
        ${ImageMagick_LIBRARIES}
        )