############################################################
# CMake Build Script for the vt_preprocess_obj_scaler executable

link_directories(${SCHISM_LIBRARY_DIRS})

include_directories(
        ${VT_INCLUDE_DIR}
)

include_directories(
        ${PROV_INCLUDE_DIR}
        ${COMMON_INCLUDE_DIR}
        ${LAMURE_CONFIG_DIR}
        ${FREEIMAGE_INCLUDE_DIR}
        ${GLFW_INCLUDE_DIRS})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR})


InitApp(${CMAKE_PROJECT_NAME}_vt_preprocess_obj_transformer)

############################################################
# Libraries
target_link_libraries(${PROJECT_NAME}
        ${PROJECT_LIBS}
        ${PROV_LIBRARY}
        ${OpenGL_LIBRARIES}
        ${GLUT_LIBRARY}
        ${VT_LIBRARY}
        optimized ${SCHISM_CORE_LIBRARY} debug ${SCHISM_CORE_LIBRARY_DEBUG}
        optimized ${SCHISM_GL_CORE_LIBRARY} debug ${SCHISM_GL_CORE_LIBRARY_DEBUG}
        optimized ${SCHISM_GL_UTIL_LIBRARY} debug ${SCHISM_GL_UTIL_LIBRARY_DEBUG}
        )
