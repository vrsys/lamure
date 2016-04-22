############################################################
# CMake Build Script for the preprocessing executable

include_directories(${XYZ_INCLUDE_DIR} 
                    ${CORE_INCLUDE_DIR})

include_directories(SYSTEM ${Boost_INCLUDE_DIR})

InitApp(${CMAKE_PROJECT_NAME}_xyz_resampler)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${XYZ_LIBRARY}
    ${CORE_LIBRARY}
    ${OpenGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    )

add_dependencies(${PROJECT_NAME} lamure_xyz lamure_core)

MsvcPostBuild(${PROJECT_NAME})
