############################################################
# CMake Build Script for the bvh_post_radii executable

InitApp(${CMAKE_PROJECT_NAME}_bvh_post_radii)

#plod essentials and cgal essentials
include_directories(${REND_INCLUDE_DIR} ${COMMON_INCLUDE_DIR} ${CGAL_INCLUDE_DIRS} ${GMP_INCLUDE_DIR} ${MPFR_INCLUDE_DIR})

############################################################
# Libraries

if (UNIX)
  SET(GMP_LIBRARIES ${GMP_LIBRARIES}/libgmp.so)
  SET(MPFR_LIBRARIES ${MPFR_LIBRARIES}/libmpfr.so)
endif(UNIX)


target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${REND_LIBRARY}
    ${OPENGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    ${SCHISM_LIBRARIES}
    ${GMP_LIBRARIES}
    ${MPFR_LIBRARIES}
    ${CGAL_LIBRARY}
    ${BOOST_LIBRARIES}
    )

#add_dependencies(${PROJECT_NAME} lamure_rendering)

