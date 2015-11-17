##############################################################################
# search paths
##############################################################################
SET(HIDAPI_INCLUDE_SEARCH_DIRS
    ${GLOBAL_EXT_DIR}/include/hidapi
)

SET(HIDAPI_LIBRARY_SEARCH_DIRS
    ${GLOBAL_EXT_DIR}/lib
)

##############################################################################
# search include and library
##############################################################################
#  HIDAPI_FOUND - System has hidapi
#  HIDAPI_INCLUDE_DIRS - The hidapi include directories
#  HIDAPI_LIBRARIES - The libraries needed to use hidapi

IF (MSVC)
  SET(_HIDAPI_LIBRARY_NAME hidapi.lib)
ELSEIF (UNIX)
  SET(_HIDAPI_LIBRARY_NAME libHIDAPI.so)
ENDIF (MSVC)

find_path(HIDAPI_INCLUDE_DIR
          NAMES hidapi.h
          PATHS ${HIDAPI_INCLUDE_SEARCH_DIRS}
         )

find_library(HIDAPI_LIBRARY
            NAMES ${_HIDAPI_LIBRARY_NAME}
            PATHS ${HIDAPI_LIBRARY_SEARCH_DIRS}
            PATH_SUFFIXES release
            )

find_library(HIDAPI_LIBRARY_DEBUG
            NAMES ${_HIDAPI_LIBRARY_NAME}
            PATHS ${HIDAPI_LIBRARY_SEARCH_DIRS}
            PATH_SUFFIXES debug
            )

##############################################################################
# verify
##############################################################################
IF ( NOT HIDAPI_INCLUDE_DIR OR NOT HIDAPI_LIBRARY OR NOT HIDAPI_LIBRARY_DEBUG )
    SET(HIDAPI_INCLUDE_SEARCH_DIR "Please provide HIDAPI include path." CACHE PATH "path to HIDAPI headers.")
    SET(HIDAPI_LIBRARY_SEARCH_DIR "Please provide HIDAPI library path." CACHE PATH "path to HIDAPI libraries.")
    MESSAGE(FATAL_ERROR "find_hidapi.cmake: unable to find HIDAPI library")
ELSE ( NOT HIDAPI_INCLUDE_DIR OR NOT HIDAPI_LIBRARY OR NOT HIDAPI_LIBRARY_DEBUG )
    UNSET(HIDAPI_INCLUDE_SEARCH_DIR CACHE)
    UNSET(HIDAPI_LIBRARY_SEARCH_DIR CACHE)
    MESSAGE(STATUS "--  found matching HIDAPI version")
ENDIF ( NOT HIDAPI_INCLUDE_DIR OR NOT HIDAPI_LIBRARY OR NOT HIDAPI_LIBRARY_DEBUG )