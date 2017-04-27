############################################################
# CMake Build Script for the rendering executable

link_directories(${SCHISM_LIBRARY_DIRS})

include_directories(${REND_INCLUDE_DIR}
                    ${COMMON_INCLUDE_DIR}
                    ${GLUT_INCLUDE_DIR}
                    ${FREEIMAGE_INCLUDE_DIR}
			        ${LAMURE_CONFIG_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIR})

InitApp(${CMAKE_PROJECT_NAME}_nvm_explorer)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${REND_LIBRARY}
    ${OpenGL_LIBRARIES} 
    ${GLUT_glut_LIBRARY}
    optimized ${SCHISM_CORE_LIBRARY} debug ${SCHISM_CORE_LIBRARY_DEBUG}
    optimized ${SCHISM_GL_CORE_LIBRARY} debug ${SCHISM_GL_CORE_LIBRARY_DEBUG}
    optimized ${SCHISM_GL_UTIL_LIBRARY} debug ${SCHISM_GL_UTIL_LIBRARY_DEBUG}
    )

add_dependencies(${PROJECT_NAME} lamure_rendering lamure_common)

MsvcPostBuild(${PROJECT_NAME})

