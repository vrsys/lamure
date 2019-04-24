############################################################
# CMake Build Script for the preprocessing executable

include_directories(${PREPROC_INCLUDE_DIR} 
                    ${COMMON_INCLUDE_DIR}
                    ${CGAL_INCLUDE_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
						   ${Boost_INCLUDE_DIR})

link_directories(${SCHISM_LIBRARY_DIRS})

InitApp(${CMAKE_PROJECT_NAME}_face_clustering)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${PREPROC_LIBRARY}
    ${OpenGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    optimized ${CGAL_LIBRARY} debug ${CGAL_LIBRARY_DEBUG}
    optimized ${CGAL_CORE_LIBRARY} debug ${CGAL_CORE_LIBRARY_DEBUG}
    )

add_dependencies(${PROJECT_NAME} lamure_preprocessing lamure_common)

MsvcPostBuild(${PROJECT_NAME})
