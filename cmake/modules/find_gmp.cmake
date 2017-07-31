##############################################################################
# search paths
##############################################################################
SET(GMP_INCLUDE_SEARCH_DIRS
  ${GLOBAL_EXT_DIR}/gmp/include
  /usr/include
  /opt/gmp/include
)

SET(GMP_LIBRARY_SEARCH_DIRS
  ${GLOBAL_EXT_DIR}/gmp/lib
  /usr/lib
  /usr/lib/x86_64-linux-gnu
  /opt/gmp/lib
)

##############################################################################
# search
##############################################################################
message(STATUS "-- checking for GMP")

SET(GMP_LIBRARY_FILENAMES "")

FOREACH (_SEARCH_DIR ${GMP_LIBRARY_SEARCH_DIRS})
    FILE(GLOB _RESULT RELATIVE ${_SEARCH_DIR} "${_SEARCH_DIR}/libgmp*" )
    IF (_RESULT) 
        FOREACH(_FILEPATH ${_RESULT})
            get_filename_component(_LIBNAME ${_FILEPATH} NAME)
            SET(GMP_LIBRARY_FILENAMES ${GMP_LIBRARY_FILENAMES} ${_LIBNAME})
        ENDFOREACH()
    ENDIF ()
ENDFOREACH()

find_path(GMP_INCLUDE_DIR NAMES gmp.h PATHS ${GMP_INCLUDE_SEARCH_DIRS})

find_library(GMP_LIBRARY NAMES gmp libGMP ${GMP_LIBRARY_FILENAMES} PATHS ${GMP_LIBRARY_SEARCH_DIRS})

##############################################################################
# verify
##############################################################################
IF ( GMP_INCLUDE_DIR AND GMP_LIBRARY)
  MESSAGE(STATUS "--  found matching GMP version")
ELSE()
  MESSAGE(FATAL_ERROR "find_GMP.cmake: unable to find GMP library.")
ENDIF ()