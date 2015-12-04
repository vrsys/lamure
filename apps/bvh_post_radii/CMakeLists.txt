############################################################
# CMake Build Script for the bvh_post_radii executable

link_directories(${SCHISM_LIBRARY_DIRS}
                 ${Boost_LIBRARY_DIR_RELEASE}
                 ${Boost_LIBRARY_DIR_DEBUG})

include_directories(${REND_INCLUDE_DIR} 
                    ${COMMON_INCLUDE_DIR} 
                    ${CGAL_INCLUDE_DIR} 
                    ${GMP_INCLUDE_DIR} 
                    ${MPFR_INCLUDE_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIR})


InitApp(${CMAKE_PROJECT_NAME}_bvh_post_radii)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${REND_LIBRARY}
    ${OpenGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    ${SCHISM_LIBRARIES}
    optimized ${CGAL_LIBRARY} debug ${CGAL_LIBRARY_DEBUG}
    optimized ${CGAL_CORE_LIBRARY} debug ${CGAL_CORE_LIBRARY_DEBUG}
    #optimized ${Boost_THREAD_LIBRARY_RELEASE} debug ${Boost_THREAD_LIBRARY_DEBUG}
    ${GMP_LIBRARY}
    ${MPFR_LIBRARY}
    )

add_dependencies(${PROJECT_NAME} lamure_preprocessing lamure_common)