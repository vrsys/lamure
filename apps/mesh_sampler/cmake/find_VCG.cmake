##############################################################################
# search paths
##############################################################################
SET(VCG_INCLUDE_SEARCH_DIRS
    ${GLOBAL_EXT_DIR}/include/vcglib
    ${THIRD_PARTY_PROJECTS_DIR}/vcglib
)

##############################################################################
# search include and library
##############################################################################
#  VCG_FOUND - System has VCG
#  VCG_INCLUDE_DIR - The VCG include directories
#  VCG_SOURCES - The source needed

find_path(VCG_INCLUDE_DIR
          NAMES vcg/complex/algorithms/halfedge_quad_clean.h
          PATHS ${VCG_INCLUDE_SEARCH_DIRS}
         )

##############################################################################
# verify
##############################################################################
IF ( NOT VCG_INCLUDE_DIR )

    SET(VCG_INCLUDE_SEARCH_DIR "Please provide VCG include path." CACHE PATH "path to VCG headers.")
    MESSAGE(FATAL_ERROR "find_VCG.cmake: unable to find VCG headers")

ELSE ( NOT VCG_INCLUDE_DIR )

    UNSET(VCG_INCLUDE_SEARCH_DIR CACHE)

    IF ( NOT VCG_SOURCES )

        file(GLOB VCG_SOURCES ${VCG_INCLUDE_DIR}/wrap/ply/*.cpp)

        IF (NOT VCG_SOURCES)
             MESSAGE(FATAL_ERROR "find_VCG.cmake: unable to find VCG sources")
        ELSE ( NOT VCG_SOURCES )
             MESSAGE(STATUS "--  found matching VCG version")
        ENDIF ( NOT VCG_SOURCES )

    ENDIF ( NOT VCG_SOURCES )

ENDIF ( NOT VCG_INCLUDE_DIR )