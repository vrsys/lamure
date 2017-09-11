##############################################################################
# search paths
##############################################################################
SET(NANOFLANN_INCLUDE_SEARCH_DIRS
    ${GLOBAL_EXT_DIR}/include/nanoflann
    ${THIRD_PARTY_PROJECTS_DIR}/nanoflann
)

##############################################################################
# search include and library
##############################################################################
#  NANOFLANN_FOUND - System has NANOFLANN
#  NANOFLANN_INCLUDE_DIR - The NANOFLANN include directories
#  NANOFLANN_SOURCES - The source needed

find_path(NANOFLANN_INCLUDE_DIR
          NAMES nanoflann.hpp
          PATHS ${NANOFLANN_INCLUDE_SEARCH_DIRS}
         )