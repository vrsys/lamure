############################################################
# CMake Build Script for the vis executable

link_directories(${SCHISM_LIBRARY_DIRS})

include_directories(
  ${REND_INCLUDE_DIR}
  ${PROV_INCLUDE_DIR}
  ${COMMON_INCLUDE_DIR}
  ${LAMURE_CONFIG_DIR}
  ${VT_INCLUDE_DIR}
  ${PVS_COMMON_INCLUDE_DIR}
  ${MESH_PREPROC_INCLUDE_DIR}
  ${FREEIMAGE_INCLUDE_DIR}
  ${GLUT_INCLUDE_DIR}
  ${GLEW_INCLUDE_DIRS}
  ${OpenGL_INCLUDE_DIRS}
  ${CGAL_INCLUDE_DIR}
)

link_directories(${OpenGL_LIBRARY_DIRS})

include_directories(SYSTEM 
  ${SCHISM_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIR})


InitApp(${CMAKE_PROJECT_NAME}_obj_texture_atlas_modifier)

############################################################
# Libraries
target_link_libraries(${PROJECT_NAME}
  ${PROJECT_LIBS}
  ${REND_LIBRARY}
  ${PVS_COMMON_LIBRARY}
  ${PROV_LIBRARY}
  ${VT_LIBRARY}
  ${MESH_PREPROC_LIBRARY}
  ${OpenGL_LIBRARY}
  ${GLUT_glut_LIBRARY}
  ${GLEW_LIBRARY}
  ${OpenGL_LIBRARIES}
  optimized ${CGAL_LIBRARY} debug ${CGAL_LIBRARY_DEBUG}
  optimized ${CGAL_CORE_LIBRARY} debug ${CGAL_CORE_LIBRARY_DEBUG}
  optimized ${SCHISM_CORE_LIBRARY} debug ${SCHISM_CORE_LIBRARY_DEBUG}
  optimized ${SCHISM_GL_CORE_LIBRARY} debug ${SCHISM_GL_CORE_LIBRARY_DEBUG}
  optimized ${SCHISM_GL_UTIL_LIBRARY} debug ${SCHISM_GL_UTIL_LIBRARY_DEBUG}
  optimized ${Boost_REGEX_LIBRARY_RELEASE} debug ${Boost_REGEX_LIBRARY_DEBUG}
)

