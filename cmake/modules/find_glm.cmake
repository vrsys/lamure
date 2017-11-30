# Additional modules
include(FindPackageHandleStandardArgs)

if (WIN32)
	# Find include files
	find_path(
		GLM_INCLUDE_DIR
		NAMES glm/glm.hpp
		PATHS
		${GLOBAL_EXT_DIR}/glm
		$ENV{PROGRAMFILES}/include
		${GLM_ROOT_DIR}/include
		DOC "The directory where glm/glm.hpp resides")
else()
	# Find include files
	find_path(
		GLM_INCLUDE_DIR
		NAMES glm/glm.hpp
		PATHS
		/usr/include
		/usr/local/include
		/sw/include
		/opt/local/include
		/opt/glm/include
		/opt/glm
		${GLM_ROOT_DIR}/include
		DOC "The directory where glm/glm.hpp resides")
endif()

# Handle REQUIRD argument, define *_FOUND variable
find_package_handle_standard_args(GLM DEFAULT_MSG GLM_INCLUDE_DIR)

# Define GLM_INCLUDE_DIRS
if (GLM_FOUND)
	set(GLM_INCLUDE_DIRS ${GLM_INCLUDE_DIR})
endif()