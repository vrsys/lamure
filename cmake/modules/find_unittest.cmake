##############################################################################
# search paths
##############################################################################
SET(UNITTEST_INCLUDE_SEARCH_DIRS
  ${GLOBAL_EXT_DIR}/include/unittest
  ${UNITTEST_INCLUDE_SEARCH_DIR}
  /usr/include
)

SET(UNITTEST_LIBRARY_SEARCH_DIRS
  ${GLOBAL_EXT_DIR}/lib
  ${UNITTEST_LIBRARY_SEARCH_DIR}
  /usr/lib
  /usr/lib/x86_64-linux-gnu
)

##############################################################################
# search
##############################################################################
message(STATUS "-- checking for UNITTEST")

find_path(UNITTEST_INCLUDE_DIR NAMES unittest++/UnitTest++.h PATHS ${UNITTEST_INCLUDE_SEARCH_DIRS})

IF (MSVC)
  find_library(UNITTEST_LIBRARY NAMES UnitTest++.lib PATHS ${UNITTEST_LIBRARY_SEARCH_DIRS} PATH_SUFFIXES release)
	find_library(UNITTEST_LIBRARY_DEBUG NAMES UnitTest++.lib UnitTest++d.lib PATHS ${UNITTEST_LIBRARY_SEARCH_DIRS} PATH_SUFFIXES debug)
ELSEIF (UNIX)
	find_library(UNITTEST_LIBRARY NAMES libUnitTest++.a PATHS ${UNITTEST_LIBRARY_SEARCH_DIRS})
ENDIF (MSVC)

##############################################################################
# verify
##############################################################################
IF ( NOT UNITTEST_INCLUDE_DIR OR NOT UNITTEST_LIBRARY OR NOT UNITTEST_LIBRARY_DEBUG )
    SET(UNITTEST_INCLUDE_SEARCH_DIR "Please provide unittest++ include path." CACHE PATH "path to unittest++ headers.")
    SET(UNITTEST_LIBRARY_SEARCH_DIR "Please provide unittest++ library path." CACHE PATH "path to unittest++ libraries.")
ELSE ( NOT UNITTEST_INCLUDE_DIR OR NOT UNITTEST_LIBRARY OR NOT UNITTEST_LIBRARY_DEBUG )
    UNSET(UNITTEST_INCLUDE_SEARCH_DIRS CACHE)
    UNSET(UNITTEST_LIBRARY_SEARCH_DIRS CACHE)
    MESSAGE(STATUS "--  found matching unittest++ version")
ENDIF ( NOT UNITTEST_INCLUDE_DIR OR NOT UNITTEST_LIBRARY OR NOT UNITTEST_LIBRARY_DEBUG )
