##############################################################################
# search paths
##############################################################################
SET(CGAL_INCLUDE_SEARCH_DIRS
  ${GLOBAL_EXT_DIR}/CGAL-4.9/include
  ${GLOBAL_EXT_DIR}/CGAL/include
  /usr/include
  /opt/cgal/include
)

SET(CGAL_LIBRARY_SEARCH_DIRS
  ${GLOBAL_EXT_DIR}/CGAL-4.9/lib
  ${GLOBAL_EXT_DIR}/CGAL/lib
  /usr/lib
  /usr/lib/x86_64-linux-gnu
  /opt/cgal/lib
)

##############################################################################
# search
##############################################################################
message(STATUS "-- checking for CGAL")

SET(CGAL_LIBRARY_FILENAME " ")
SET(CGAL_LIBRARY_DEBUG_FILENAME " ")
SET(CGAL_CORE_LIBRARY_FILENAME " ")
SET(CGAL_CORE_LIBRARY_DEBUG_FILENAME " ")

FOREACH (_SEARCH_DIR ${CGAL_LIBRARY_SEARCH_DIRS})

  IF (UNIX)
    FILE(GLOB _RESULT RELATIVE ${_SEARCH_DIR} "${_SEARCH_DIR}/libCGAL.*" )
  ELSEIF(MSVC)
    FILE(GLOB _RESULT RELATIVE ${_SEARCH_DIR} "${_SEARCH_DIR}/libCGAL*" )
  ENDIF()
    IF (_RESULT) 
        FOREACH(_FILEPATH ${_RESULT})
            get_filename_component(_LIBNAME ${_FILEPATH} NAME)
            if (${_FILEPATH} MATCHES ".*-gd-.*")
                SET(CGAL_LIBRARY_DEBUG_FILENAME ${CGAL_LIBRARY_DEBUG_FILENAME} ${_LIBNAME})
            else ()
                SET(CGAL_LIBRARY_FILENAME ${CGAL_LIBRARY_FILENAME} ${_LIBNAME})
            endif ()
        ENDFOREACH()
    ENDIF ()

    FILE(GLOB _RESULT RELATIVE ${_SEARCH_DIR} "${_SEARCH_DIR}/libCGAL_Core*" )
    IF (_RESULT) 
        FOREACH(_FILEPATH ${_RESULT})
            get_filename_component(_LIBNAME ${_FILEPATH} NAME)
            if (${_FILEPATH} MATCHES ".*-gd-.*")
                SET(CGAL_CORE_LIBRARY_DEBUG_FILENAME ${CGAL_CORE_LIBRARY_DEBUG_FILENAME} ${_LIBNAME})
            else ()
                SET(CGAL_CORE_LIBRARY_FILENAME ${CGAL_CORE_LIBRARY_FILENAME} ${_LIBNAME})
            endif ()
        ENDFOREACH()
    ENDIF ()
ENDFOREACH()

find_path(CGAL_INCLUDE_DIR NAMES CGAL/CGAL_Ipelet_base.h PATHS ${CGAL_INCLUDE_SEARCH_DIRS})

find_library(CGAL_LIBRARY NAMES ${CGAL_LIBRARY_FILENAME} PATHS ${CGAL_LIBRARY_SEARCH_DIRS})
find_library(CGAL_CORE_LIBRARY NAMES ${CGAL_CORE_LIBRARY_FILENAME} PATHS ${CGAL_LIBRARY_SEARCH_DIRS})

get_filename_component(_CGAL_LIBRARY_DIR ${CGAL_LIBRARY} PATH)
SET(CGAL_LIBRARY_DIR "${_CGAL_LIBRARY_DIR}/" CACHE PATH "Sets path to CGAL libraries")

IF (UNIX)
  UNSET(CGAL_CORE_LIBRARY_DEBUG)
  UNSET(CGAL_LIBRARY_DEBUG)
  SET(CGAL_CORE_LIBRARY_DEBUG ${CGAL_CORE_LIBRARY} CACHE FILEPATH "Sets path to CGAL Core library debug")
  SET(CGAL_LIBRARY_DEBUG ${CGAL_LIBRARY} CACHE FILEPATH "Sets path to CGAL library debug")
ELSEIF(MSVC)
  find_library(CGAL_LIBRARY_DEBUG NAMES ${CGAL_LIBRARY_DEBUG_FILENAME} PATHS ${CGAL_LIBRARY_SEARCH_DIRS})
  find_library(CGAL_CORE_LIBRARY_DEBUG NAMES ${CGAL_CORE_LIBRARY_DEBUG_FILENAME} PATHS ${CGAL_LIBRARY_SEARCH_DIRS})
ENDIF()

##############################################################################
# verify
##############################################################################
IF ( CGAL_LIBRARY AND CGAL_INCLUDE_DIR)
  MESSAGE(STATUS "--  found matching CGAL version")
ELSE ()
  MESSAGE(FATAL_ERROR "find_CGAL.cmake: unable to find CGAL library.")
ENDIF ()