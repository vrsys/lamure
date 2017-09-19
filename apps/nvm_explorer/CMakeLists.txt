############################################################
# CMake Build Script for the bvh_extractor executable

link_directories(${SCHISM_LIBRARY_DIRS})

include_directories(${REND_INCLUDE_DIR}
        			${PROV_INCLUDE_DIR}
					${COMMON_INCLUDE_DIR}
					${LAMURE_CONFIG_DIR}
					${FREEIMAGE_INCLUDE_DIR}
					${GLFW_INCLUDE_DIRS}
					${GLEW_INCLUDE_DIRS}
					${OpenGL_INCLUDE_DIRS}
					)

link_directories(${OpenGL_LIBRARY_DIRS})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
		${Boost_INCLUDE_DIR})


InitApp(${CMAKE_PROJECT_NAME}_nvm_explorer)

############################################################
# Libraries
target_link_libraries(${PROJECT_NAME}
		${PROJECT_LIBS}
		${REND_LIBRARY}
        ${PROV_LIBRARY}
		${OpenGL_LIBRARY}
		${GLFW_LIBRARIES}
		${GLEW_LIBRARY}
		${OPENGL_LIBRARY}
		# ${GLUT_LIBRARY}
    optimized ${SCHISM_CORE_LIBRARY} debug ${SCHISM_CORE_LIBRARY_DEBUG}
    optimized ${SCHISM_GL_CORE_LIBRARY} debug ${SCHISM_GL_CORE_LIBRARY_DEBUG}
    optimized ${SCHISM_GL_UTIL_LIBRARY} debug ${SCHISM_GL_UTIL_LIBRARY_DEBUG}
		)	