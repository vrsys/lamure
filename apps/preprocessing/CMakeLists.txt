############################################################
# CMake Build Script for the preprocessing executable

include_directories(${LAMURE_XYZ_INCLUDE_DIR} 
                    ${LAMURE_CORE_INCLUDE_DIR})

include_directories(SYSTEM ${Boost_INCLUDE_DIR})

InitApp(${CMAKE_PROJECT_NAME}_preprocessing)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${LAMURE_XYZ_LIBRARY}
    ${LAMURE_CORE_LIBRARY}
    ${OpenGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    )

add_dependencies(${PROJECT_NAME} lamure_xyz lamure_core)

MsvcPostBuild(${PROJECT_NAME})
