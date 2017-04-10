############################################################
# CMake Build Script for the rendering executable

include_directories(${LAMURE_CORE_INCLUDE_DIR}
                    ${LAMURE_GL_INCLUDE_DIR}
                    ${LAMURE_LOD_INCLUDE_DIR}
                    ${GLUT_INCLUDE_DIR}
                    ${FREEIMAGE_INCLUDE_DIR}
                    ${LAMURE_CONFIG_DIR})

include_directories(SYSTEM ${Boost_INCLUDE_DIR})

InitApp(${CMAKE_PROJECT_NAME}_rendering)

############################################################
# Libraries

#message(ERROR "hello ${GL_LIBRARY} ${CORE_LIBRARY}")

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    "GL"
    "GLEW"
    ${OpenGL_LIBRARIES} 
    ${GLUT_glut_LIBRARY}
    ${LAMURE_GL_LIBRARY}
    ${LAMURE_CORE_LIBRARY}
    ${LAMURE_XYZ_LIBRARY}
    ${LAMURE_LOD_LIBRARY}
    )

add_dependencies(${PROJECT_NAME} lamure_lod lamure_gl lamure_core)

MsvcPostBuild(${PROJECT_NAME})

