############################################################
# CMake Build Script

############################################################
# Project settings
cmake_minimum_required(VERSION 2.8.7)
project(mesh_sampler)

############################################################
# Project files

file(GLOB PROJECT_INCLUDES ${PROJECT_SOURCE_DIR}/*.h)
file(GLOB PROJECT_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp) 

include_directories(${PROJECT_SOURCE_DIR})

option(LAMURE_ENABLE_MESH_SAMPLER "Enable mesh sampler application."                OFF)

IF (LAMURE_ENABLE_MESH_SAMPLER)

############################################################
# Externals

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include (find_VCG)

# VCG library
list(APPEND PROJECT_SOURCES ${VCG_SOURCES})

include_directories(${VCG_INCLUDE_DIR} ${FREEIMAGE_INCLUDE_DIR})

############################################################
# Executables
set(SAMPLER_EXE ${CMAKE_PROJECT_NAME}_${PROJECT_NAME})
set(SAMPLER_EXE_WITH_WEDGES ${CMAKE_PROJECT_NAME}_${PROJECT_NAME}_w)

# with vertex normals
add_executable(${SAMPLER_EXE} ${PROJECT_INCLUDES} ${PROJECT_SOURCES})

IF (MSVC)
  set(PROJECT_LIBS winmm.lib Imm32.lib Version.lib )
ELSEIF (UNIX)
  set(PROJECT_LIBS pthread)
ENDIF (MSVC)

target_link_libraries(${SAMPLER_EXE} 
                      ${PROJECT_LIBS} 
                      optimized ${FREEIMAGE_LIBRARY} 
                      optimized ${FREEIMAGEPLUS_LIBRARY} 
                      debug ${FREEIMAGE_LIBRARY_DEBUG}
                      debug ${FREEIMAGEPLUS_LIBRARY_DEBUG}
                      )

MsvcPostBuild(${SAMPLER_EXE})


# with wedge normals

add_executable(${SAMPLER_EXE_WITH_WEDGES} ${PROJECT_INCLUDES} ${PROJECT_SOURCES})
set_target_properties(${SAMPLER_EXE_WITH_WEDGES} PROPERTIES COMPILE_DEFINITIONS "USE_WEDGE_NORMALS")

target_link_libraries(${SAMPLER_EXE_WITH_WEDGES} 
                      ${PROJECT_LIBS}
                      optimized ${FREEIMAGE_LIBRARY} 
                      optimized ${FREEIMAGEPLUS_LIBRARY} 
                      debug ${FREEIMAGE_LIBRARY_DEBUG}
                      debug ${FREEIMAGEPLUS_LIBRARY_DEBUG})

MsvcPostBuild(${SAMPLER_EXE_WITH_WEDGES})

###############################################################################
# install 
###############################################################################

IF (MSVC)
  install (TARGETS ${SAMPLER_EXE} ${SAMPLER_EXE_WITH_WEDGES}
    CONFIGURATIONS Release
    RUNTIME DESTINATION bin/Release
    LIBRARY DESTINATION lib/Release
    ARCHIVE DESTINATION lib/Release
  )

  install (TARGETS ${SAMPLER_EXE} ${SAMPLER_EXE_WITH_WEDGES}
    CONFIGURATIONS Debug
    RUNTIME DESTINATION bin/Debug
    LIBRARY DESTINATION lib/Debug
    ARCHIVE DESTINATION lib/Debug
  )

  install(DIRECTORY ${GLOBAL_EXT_DIR}/bin DESTINATION . FILES_MATCHING PATTERN "*.dll")
  install(DIRECTORY ${GLOBAL_EXT_DIR}/bin DESTINATION . FILES_MATCHING PATTERN "*.exe")

ELSEIF (UNIX)
  install (TARGETS ${SAMPLER_EXE} ${SAMPLER_EXE_WITH_WEDGES}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
ENDIF (MSVC)

ENDIF (LAMURE_ENABLE_MESH_SAMPLER)
