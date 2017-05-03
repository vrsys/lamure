############################################################
# CMake Build Script for the bvh_extractor executable

link_directories(${SCHISM_LIBRARY_DIRS})

include_directories(${REND_INCLUDE_DIR}
		${COMMON_INCLUDE_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
		${Boost_INCLUDE_DIR})


InitApp(${CMAKE_PROJECT_NAME}_nvm_explorer)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
		${PROJECT_LIBS}
		${REND_LIBRARY}
		${OpenGL_LIBRARIES}
		${GLUT_LIBRARY}
		)