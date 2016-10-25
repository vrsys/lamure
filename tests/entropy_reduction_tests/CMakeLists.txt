############################################################
# CMake Build Script for the preprocessing executable

include_directories(${PREPROC_INCLUDE_DIR} 
                    ${COMMON_INCLUDE_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
		           ${Boost_INCLUDE_DIR}
 		           ${CMAKE_SOURCE_DIR}/third_party)

link_directories(${SCHISM_LIBRARY_DIRS})

InitTest(${CMAKE_PROJECT_NAME}_entropy_reduction_tests)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${PREPROC_LIBRARY}
    )

add_dependencies(${PROJECT_NAME} lamure_preprocessing lamure_common)

MsvcPostBuild(${PROJECT_NAME})
