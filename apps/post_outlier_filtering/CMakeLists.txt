############################################################
# CMake Build Script for the post_outlier_filtering executable

link_directories(${SCHISM_LIBRARY_DIRS})

include_directories(${REND_INCLUDE_DIR}
					${PREPROC_INCLUDE_DIR}
                    ${COMMON_INCLUDE_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
						   ${Boost_INCLUDE_DIR})


InitApp(${CMAKE_PROJECT_NAME}_post_outlier_filtering)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${REND_LIBRARY}
    ${OpenGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    )

