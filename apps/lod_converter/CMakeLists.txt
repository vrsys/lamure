############################################################
# CMake Build Script for the lod_converter executable

InitApp(${CMAKE_PROJECT_NAME}_lod_converter)

include_directories(${REND_INCLUDE_DIR} ${COMMON_INCLUDE_DIR})

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${REND_LIBRARY}
    ${OPENGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    ${SCHISM_LIBRARIES}
    ${BOOST_LIBRARIES}
    )

#add_dependencies(${PROJECT_NAME} lamure_rendering)

