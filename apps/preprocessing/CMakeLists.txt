############################################################
# CMake Build Script for the preprocessing executable

InitApp(${CMAKE_PROJECT_NAME}_preprocessing)

include_directories(${PREPROC_INCLUDE_DIR} ${COMMON_INCLUDE_DIR})

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${PREPROC_LIBRARY}
    ${OPENGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    ${SCHISM_LIBRARIES}
    ${BOOST_LIBRARIES}
    )

add_dependencies(${PROJECT_NAME} lamure_preprocessing)

MsvcPostBuild()

