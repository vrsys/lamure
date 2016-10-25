############################################################
# CMake Build Script for the bvh_modifier executable

link_directories(${SCHISM_LIBRARY_DIRS})

include_directories(${PREPROC_INCLUDE_DIR} 
                    ${COMMON_INCLUDE_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIR})

InitApp(${CMAKE_PROJECT_NAME}_bvh_modifier)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${PREPROC_LIBRARY}
    ${OpenGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    optimized ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE} debug ${Boost_PROGRAM_OPTIONS_LIBRARY_DEBUG}
    )

if (${LAMURE_USE_CGAL_FOR_NNI})
  target_link_libraries(${PROJECT_NAME}
      ${GMP_LIBRARY}
      ${MPFR_LIBRARY}
      optimized ${CGAL_LIBRARY} debug ${CGAL_LIBRARY_DEBUG}
      optimized ${CGAL_CORE_LIBRARY} debug ${CGAL_CORE_LIBRARY_DEBUG}
      )
endif ()

add_dependencies(${PROJECT_NAME} lamure_preprocessing lamure_common)

