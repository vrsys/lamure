##############################################################################
# search paths
##############################################################################
SET(PLY_INCLUDE_SEARCH_DIRS
    ${GLOBAL_EXT_DIR}/include/libply
)

SET(PLY_LIBRARY_SEARCH_DIRS
    ${GLOBAL_EXT_DIR}/lib
)

##############################################################################
# search include and library
##############################################################################
#  PLY_FOUND - System has ply
#  PLY_INCLUDE_DIRS - The ply include directories
#  PLY_LIBRARIES - The libraries needed to use ply

IF (MSVC)
  SET(_PLY_LIBRARY_NAME ply.lib)
ELSEIF (UNIX)
  SET(_PLY_LIBRARY_NAME libply.so)
ENDIF (MSVC)

find_path(PLY_INCLUDE_DIR
          NAMES ply.hpp
          PATHS ${PLY_INCLUDE_SEARCH_DIRS}
         )

find_library(PLY_LIBRARY
            NAMES ${_PLY_LIBRARY_NAME}
            PATHS ${PLY_LIBRARY_SEARCH_DIRS}
            PATH_SUFFIXES release
            )

find_library(PLY_LIBRARY_DEBUG
            NAMES ${_PLY_LIBRARY_NAME}
            PATHS ${PLY_LIBRARY_SEARCH_DIRS}
            PATH_SUFFIXES debug
            )

##############################################################################
# verify
##############################################################################
IF ( NOT PLY_INCLUDE_DIR OR NOT PLY_LIBRARY OR NOT PLY_LIBRARY_DEBUG )
    SET(PLY_INCLUDE_SEARCH_DIR "Please provide ply include path." CACHE PATH "path to ply headers.")
    SET(PLY_LIBRARY_SEARCH_DIR "Please provide ply library path." CACHE PATH "path to ply libraries.")
    MESSAGE(FATAL_ERROR "find_ply.cmake: unable to find ply library")
ELSE ( NOT PLY_INCLUDE_DIR OR NOT PLY_LIBRARY OR NOT PLY_LIBRARY_DEBUG )
    UNSET(PLY_INCLUDE_SEARCH_DIR CACHE)
    UNSET(PLY_LIBRARY_SEARCH_DIR CACHE)
    MESSAGE(STATUS "--  found matching ply version")
ENDIF ( NOT PLY_INCLUDE_DIR OR NOT PLY_LIBRARY OR NOT PLY_LIBRARY_DEBUG )
