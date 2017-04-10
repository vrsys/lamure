############################################################
# CMake Build Script for the preprocessing executable

include_directories(${LAMURE_XYZ_INCLUDE_DIR} 
                    ${LAMURE_CORE_INCLUDE_DIR})

include_directories(SYSTEM ${Boost_INCLUDE_DIR}
                           ${CMAKE_SOURCE_DIR}/third_party)

InitTest(${CMAKE_PROJECT_NAME}_surfel_tests)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${LAMURE_XYZ_LIBRARY}
    ${LAMURE_CORE_LIBRARY}
    )

add_dependencies(${PROJECT_NAME} lamure_xyz lamure_core)

MsvcPostBuild(${PROJECT_NAME})
