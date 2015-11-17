##############################################################################
# search paths
##############################################################################
SET(L4C_INCLUDE_SEARCH_DIRS
    ${GLOBAL_EXT_DIR}/include
)

SET(L4C_LIBRARY_SEARCH_DIRS
    ${GLOBAL_EXT_DIR}/lib
)

##############################################################################
# search include and library
##############################################################################
#  L4C_FOUND - System has log4cpp
#  L4C_INCLUDE_DIRS - The log4cpp include directories
#  L4C_LIBRARIES - The libraries needed to use log4cpp

IF (MSVC)
  SET(_L4C_LIBRARY_NAME log4cplus.lib log4cplusD.lib)
ELSEIF (UNIX)
  SET(_L4C_LIBRARY_NAME log4cplus.so)
ENDIF (MSVC)

find_path(L4C_INCLUDE_DIR
          NAMES log4cplus/log4judpappender.h
          PATHS ${L4C_INCLUDE_SEARCH_DIRS}
         )

find_library(L4C_LIBRARY
            NAMES ${_L4C_LIBRARY_NAME}
            PATHS ${L4C_LIBRARY_SEARCH_DIRS}
            PATH_SUFFIXES release
            )

find_library(L4C_LIBRARY_DEBUG
            NAMES ${_L4C_LIBRARY_NAME}
            PATHS ${L4C_LIBRARY_SEARCH_DIRS}
            PATH_SUFFIXES debug
            )

IF (L4C_LIBRARY_DEBUG AND L4C_LIBRARY AND L4C_INCLUDE_DIR)
    MESSAGE(STATUS "--  found matching version")  
ELSE (L4C_LIBRARY_DEBUG AND L4C_LIBRARY AND L4C_INCLUDE_DIR)
    MESSAGE(FATAL_ERROR "find_ply.cmake: unable to find ply library")
ENDIF (L4C_LIBRARY_DEBUG AND L4C_LIBRARY AND L4C_INCLUDE_DIR)
    
##############################################################################
# verify
##############################################################################
IF ( NOT L4C_INCLUDE_DIR OR NOT L4C_LIBRARY OR NOT L4C_LIBRARY_DEBUG )
    SET(L4C_INCLUDE_SEARCH_DIR "Please provide log4cpp include path." CACHE PATH "path to log4cpp headers.")
    SET(L4C_LIBRARY_SEARCH_DIR "Please provide log4cpp library path." CACHE PATH "path to log4cpp libraries.")
ELSE ( NOT L4C_INCLUDE_DIR OR NOT L4C_LIBRARY OR NOT L4C_LIBRARY_DEBUG )
    UNSET(L4C_INCLUDE_SEARCH_DIR CACHE)
    UNSET(L4C_LIBRARY_SEARCH_DIR CACHE)
    MESSAGE(STATUS "--  found matching log4cpp version")
ENDIF ( NOT L4C_INCLUDE_DIR OR NOT L4C_LIBRARY OR NOT L4C_LIBRARY_DEBUG )
