############################################################
# CMake Build Script for the image_based_error_measurement executable

include_directories(${CORE_INCLUDE_DIR})
include_directories(${QUALITY_MEASUREMENT_INCLUDE_DIR})
include_directories(${FREEIMAGE_INCLUDE_DIR})
include_directories(SYSTEM ${Boost_INCLUDE_DIR})

InitApp(${CMAKE_PROJECT_NAME}_image_based_error_measurement)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${CORE_LIBRARY}
    ${QUALITY_MEASUREMENT_LIBRARY}
    )

add_dependencies(${PROJECT_NAME} lamure_quality_measurement)

MsvcPostBuild(${PROJECT_NAME})
