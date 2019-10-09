############################################################
# CMake Build Script

############################################################
# Project settings
cmake_minimum_required(VERSION 2.8.7)
project(mesh_fixer)

############################################################
# Project files

file(GLOB PROJECT_INCLUDES ${PROJECT_SOURCE_DIR}/*.h)
file(GLOB PROJECT_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)

include_directories(${PROJECT_SOURCE_DIR})

############################################################
# Externals

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(find_VCG)

# VCG library
list(APPEND PROJECT_SOURCES ${VCG_SOURCES})

include_directories(${VCG_INCLUDE_DIR} ${FREEIMAGE_INCLUDE_DIR})

############################################################
# Executables
set(FIXER_EXE ${CMAKE_PROJECT_NAME}_${PROJECT_NAME})

# with vertex normals
add_executable(${FIXER_EXE} ${PROJECT_INCLUDES} ${PROJECT_SOURCES})

IF (MSVC)
    set(PROJECT_LIBS winmm.lib Imm32.lib Version.lib)
ELSEIF (UNIX)
    set(PROJECT_LIBS pthread)
ENDIF (MSVC)

target_link_libraries(${FIXER_EXE}
        ${PROJECT_LIBS}
        optimized ${FREEIMAGE_LIBRARY}
        optimized ${FREEIMAGEPLUS_LIBRARY}
        debug ${FREEIMAGE_LIBRARY_DEBUG}
        debug ${FREEIMAGEPLUS_LIBRARY_DEBUG}
        )

IF (MSVC)
    MsvcPostBuild(${FIXER_EXE})
ENDIF (MSVC)

###############################################################################
# install 
###############################################################################

IF (MSVC)
    install(TARGETS ${FIXER_EXE}
            CONFIGURATIONS Release
            RUNTIME DESTINATION bin/Release
            LIBRARY DESTINATION lib/Release
            ARCHIVE DESTINATION lib/Release
            )

    install(TARGETS ${FIXER_EXE}
            CONFIGURATIONS Debug
            RUNTIME DESTINATION bin/Debug
            LIBRARY DESTINATION lib/Debug
            ARCHIVE DESTINATION lib/Debug
            )

    install(DIRECTORY ${GLOBAL_EXT_DIR}/bin DESTINATION . FILES_MATCHING PATTERN "*.dll")
    install(DIRECTORY ${GLOBAL_EXT_DIR}/bin DESTINATION . FILES_MATCHING PATTERN "*.exe")

ELSEIF (UNIX)
    install(TARGETS ${FIXER_EXE}
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            )
ENDIF (MSVC)
