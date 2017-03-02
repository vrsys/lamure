############################################################
# CMake Build Script for the potentially_visible_set grid converter executable

link_directories(${SCHISM_LIBRARY_DIRS})

include_directories(${PVS_COMMON_INCLUDE_DIR}
                    ${PVS_PREPROCESSING_INCLUDE_DIR}
                    ${REND_INCLUDE_DIR}
                    ${COMMON_INCLUDE_DIR}
                    ${GLUT_INCLUDE_DIR}
                    ${FREEIMAGE_INCLUDE_DIR}
			        ${LAMURE_CONFIG_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
						   ${Boost_INCLUDE_DIR})

link_directories(${SCHISM_LIBRARY_DIRS})

InitApp(${CMAKE_PROJECT_NAME}_potentially_visible_set_bounding_extrapolator)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${PVS_COMMON_LIBRARY}
    optimized ${SCHISM_CORE_LIBRARY} debug ${SCHISM_CORE_LIBRARY_DEBUG}
    )

add_dependencies(${PROJECT_NAME} lamure_potentially_visible_set_common lamure_common)

MsvcPostBuild(${PROJECT_NAME})
