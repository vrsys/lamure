############################################################
# CMake Build Script for the bvh_modifier executable

include_directories(${LAMURE_XYZ_INCLUDE_DIR} 
                    ${LAMURE_CORE_INCLUDE_DIR}
                    ${LAMURE_GL_INCLUDE_DIR})

include_directories(SYSTEM ${Boost_INCLUDE_DIR})

InitApp(${CMAKE_PROJECT_NAME}_bvh_modifier)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${LAMURE_XYZ_LIBRARY}
    ${LAMURE_CORE_LIBRARY}
    ${LAMURE_GL_LIBRARY}
    ${OpenGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    optimized ${CGAL_LIBRARY} debug ${CGAL_LIBRARY_DEBUG}
    optimized ${CGAL_CORE_LIBRARY} debug ${CGAL_CORE_LIBRARY_DEBUG}
    optimized ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE} debug ${Boost_PROGRAM_OPTIONS_LIBRARY_DEBUG}
    )

add_dependencies(${PROJECT_NAME} lamure_xyz lamure_core)

