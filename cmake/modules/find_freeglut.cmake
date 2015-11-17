##############################################################################FREE
# search paths
##############################################################################
SET(GLUT_INCLUDE_SEARCH_DIRS
  ${GLOBAL_EXT_DIR}/include/freeglut
  ${GLUT_INCLUDE_SEARCH_DIR}
  /usr/include
)

SET(GLUT_LIBRARY_SEARCH_DIRS
  ${GLOBAL_EXT_DIR}/lib
  ${GLUT_LIBRARY_SEARCH_DIR}
  /usr/lib
  /usr/lib/x86_64-linux-gnu
)

##############################################################################
# search
##############################################################################
message(STATUS "-- checking for GLUT")

find_path(GLUT_INCLUDE_DIRS NAMES GL/freeglut.h PATHS ${GLUT_INCLUDE_SEARCH_DIRS})

IF (MSVC)
	find_library(GLUT_LIBRARY NAMES freeglut.lib PATHS ${GLUT_LIBRARY_SEARCH_DIRS} PATH_SUFFIXES release)
	find_library(GLUT_LIBRARY_DEBUG NAMES freeglut.lib freeglutd.lib PATHS ${GLUT_LIBRARY_SEARCH_DIRS} PATH_SUFFIXES debug)
ELSEIF (UNIX)
	find_library(GLUT_LIBRARY NAMES libglut.so PATHS ${GLUT_LIBRARY_SEARCH_DIRS})
ENDIF (MSVC)

##############################################################################
# verify
##############################################################################
IF ( NOT GLUT_INCLUDE_DIRS OR NOT GLUT_LIBRARY OR NOT GLUT_LIBRARY_DEBUG)
    IF ( NOT GLUT_INCLUDE_DIRS ) 
      SET(GLUT_INCLUDE_SEARCH_DIR "Please provide freeglut include path." CACHE PATH "path to freeglut headers.")
    ENDIF ( NOT GLUT_LIBRARY OR NOT GLUT_LIBRARY_DEBUG ) 
    IF ( NOT GLUT_INCLUDE_DIRS ) 
      SET(GLUT_LIBRARY_SEARCH_DIR "Please provide freeglut library path." CACHE PATH "path to freeglut libraries.")
    ENDIF ( NOT GLUT_LIBRARY OR NOT GLUT_LIBRARY_DEBUG ) 
ELSE ( NOT GLUT_INCLUDE_DIRS OR NOT GLUT_LIBRARY OR NOT GLUT_LIBRARY_DEBUG)
    UNSET(GLUT_INCLUDE_SEARCH_DIR CACHE)
    UNSET(GLUT_LIBRARY_SEARCH_DIR CACHE)
    MESSAGE(STATUS "--  found matching freeglut version")
ENDIF ( NOT GLUT_INCLUDE_DIRS OR NOT GLUT_LIBRARY OR NOT GLUT_LIBRARY_DEBUG)
