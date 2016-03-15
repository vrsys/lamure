############################################################
# CMake Build Script for the bvh_post_normals_adaptive_per_surfel executable

link_directories(${SCHISM_LIBRARY_DIRS})

include_directories(${REND_INCLUDE_DIR} 
                    ${COMMON_INCLUDE_DIR} 
                    ${CGAL_INCLUDE_DIRS} 
                    ${GMP_INCLUDE_DIR} 
                    ${MPFR_INCLUDE_DIR}
                    ${SCHISM_INCLUDE_DIRS}
                    ${Boost_INCLUDE_DIR})

InitApp(${CMAKE_PROJECT_NAME}_bvh_post_normals_adaptive_per_surfel)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${REND_LIBRARY}
    ${OpenGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    ${SCHISM_LIBRARIES}
    ${GMP_LIBRARIES}
    ${MPFR_LIBRARIES}
    )

add_dependencies(${PROJECT_NAME} lamure_preprocessing lamure_common)

