############################################################
# CMake Build Script for the preprocessing executable

include_directories(${PREPROC_INCLUDE_DIR} 
                    ${COMMON_INCLUDE_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
						   ${Boost_INCLUDE_DIR})

link_directories(${SCHISM_LIBRARY_DIRS})

InitApp(${CMAKE_PROJECT_NAME}_xyz_resampler)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${PREPROC_LIBRARY}
    ${OpenGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    )

add_dependencies(${PROJECT_NAME} lamure_preprocessing lamure_common)

MsvcPostBuild(${PROJECT_NAME})