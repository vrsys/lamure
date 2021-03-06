IF(NOT MSVC)

############################################################
# CMake Build Script for the pcl_muxxer executable

include_directories(${PREPROC_INCLUDE_DIR} 
                    ${COMMON_INCLUDE_DIR}
                    ${CGAL_INCLUDE_DIR}
                    ${REND_INCLUDE_DIR}
                    ${MESH_PREPROC_INCLUDE_DIR}
                    ${GLEW_INCLUDE_DIRS}
                    ${VCG_INCLUDE_DIR})

include_directories(SYSTEM ${SCHISM_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIR})

link_directories(${SCHISM_LIBRARY_DIRS})

InitApp(${CMAKE_PROJECT_NAME}_pcl_muxxer)

############################################################
# Libraries

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_LIBS}
    ${PREPROC_LIBRARY}
    ${REND_LIBRARY}
    ${MESH_PREPROC_LIBRARY}
    optimized ${CGAL_LIBRARY} debug ${CGAL_LIBRARY_DEBUG}
    optimized ${CGAL_CORE_LIBRARY} debug ${CGAL_CORE_LIBRARY_DEBUG}
    "GL"
    "GLEW"
    ${OPENGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    ${OpenGL_LIBRARIES} 
    ${GLUT_LIBRARY}
    ${GLEW_LIBRARY}
    ${GLEW_LIBRARIES}
    optimized ${Boost_SERIALIZATION_LIBRARY_RELEASE} debug ${Boost_SERIALIZATION_LIBRARY_DEBUG}
    optimized ${Boost_THREAD_LIBRARY_RELEASE} debug ${Boost_THREAD_LIBRARY_DEBUG}
    stdc++fs
    )

add_dependencies(${PROJECT_NAME} lamure_mesh_preprocessing lamure_preprocessing lamure_common)

# MsvcPostBuild(${PROJECT_NAME})

ENDIF(NOT MSVC)
