############################################################
# CMake Build Script for the rendering executable

InitApp(${CMAKE_PROJECT_NAME}_rendering)

include_directories(${REND_INCLUDE_DIR}
                    ${COMMON_INCLUDE_DIR}
                    ${GLUT_INCLUDE_DIRS}
                    ${FREEIMAGE_INCLUDE_DIR}
					          ${LAMURE_CONFIG_DIR})

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${REND_LIBRARY}
    ${OpenGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    ${SCHISM_LIBRARIES}
    ${BOOST_LIBRARIES}
    )

add_dependencies(${PROJECT_NAME} lamure_rendering lamure_common)

MsvcPostBuild(${PROJECT_NAME})

