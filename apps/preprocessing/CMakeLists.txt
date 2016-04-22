############################################################
# CMake Build Script for the preprocessing executable

include_directories(${PREPROC_INCLUDE_DIR} 
                    ${CORE_INCLUDE_DIR})

include_directories(SYSTEM ${Boost_INCLUDE_DIR})

InitApp(${CMAKE_PROJECT_NAME}_preprocessing)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${PREPROC_LIBRARY}
    ${CORE_LIBRARY}
    ${OpenGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    ${SCHISM_LIBRARIES}
    )

add_dependencies(${PROJECT_NAME} lamure_preprocessing lamure_core)

MsvcPostBuild(${PROJECT_NAME})
