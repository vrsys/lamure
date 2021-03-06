############################################################
# CMake Build Script for the rendering executable

link_directories(${SCHISM_LIBRARY_DIRS})

include_directories(${REND_INCLUDE_DIR}
                    ${COMMON_INCLUDE_DIR}
                    ${PVS_COMMON_INCLUDE_DIR}
                    ${GLUT_INCLUDE_DIR}
		    ${GLM_INCLUDE_DIR}
                    ${GLEW_INCLUDE_DIRS}
                    ${FREEIMAGE_INCLUDE_DIR}
	            ${LAMURE_CONFIG_DIR}
                    ${CGAL_INCLUDE_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIR})

InitApp(${CMAKE_PROJECT_NAME}_obj)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${REND_LIBRARY}
    ${PVS_COMMON_LIBRARY}
    ${OpenGL_LIBRARIES}
    ${GLEW_LIBRARY}
    ${GLUT_glut_LIBRARY}
	${GLUT_LIBRARY}
    optimized ${SCHISM_CORE_LIBRARY} debug ${SCHISM_CORE_LIBRARY_DEBUG}
    optimized ${SCHISM_GL_CORE_LIBRARY} debug ${SCHISM_GL_CORE_LIBRARY_DEBUG}
    optimized ${SCHISM_GL_UTIL_LIBRARY} debug ${SCHISM_GL_UTIL_LIBRARY_DEBUG}
    optimized ${Boost_REGEX_LIBRARY_RELEASE} debug ${Boost_REGEX_LIBRARY_DEBUG}
    optimized ${CGAL_LIBRARY} debug ${CGAL_LIBRARY_DEBUG}
    optimized ${CGAL_CORE_LIBRARY} debug ${CGAL_CORE_LIBRARY_DEBUG}
    )

add_dependencies(${PROJECT_NAME} lamure_virtual_texturing lamure_rendering lamure_common lamure_pvs_common)

MsvcPostBuild(${PROJECT_NAME})

