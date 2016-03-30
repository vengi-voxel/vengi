if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(TOOLS_DIR ${FIPS_PROJECT_DIR}/tools/win32)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(TOOLS_DIR ${FIPS_PROJECT_DIR}/tools/osx)
else()
	set(TOOLS_DIR ${FIPS_PROJECT_DIR}/tools/linux)
endif()

macro(check_glsl_files TARGET)
	set(files ${ARGV})
	list(REMOVE_AT files 0)
	find_program(GLSL_VALIDATOR_EXECUTABLE NAMES glslangValidator PATHS ${TOOLS_DIR})
	if (GLSL_VALIDATOR_EXECUTABLE)
		message("${GLSL_VALIDATOR_EXECUTABLE} found - executing in ${FIPS_PROJECT_DIR}/data/${TARGET}/shaders")
		foreach(_file ${files})
			add_custom_target(
				${_file}
				COMMENT "Validate ${_file}"
				COMMAND ${FIPS_DEPLOY_DIR}/${CMAKE_PROJECT_NAME}/${FIPS_CONFIG}/shadertool ${GLSL_VALIDATOR_EXECUTABLE} ${_file}
				DEPENDS shadertool
				WORKING_DIRECTORY ${FIPS_PROJECT_DIR}/data/${TARGET}/shaders
			)
			add_dependencies(${TARGET} shadertool ${_file} ${_outfile})
		endforeach()
	else()
		message(WARNING "No ${GLSL_VALIDATOR_EXECUTABLE} found")
	endif()
endmacro()

#
# macro for the FindLibName.cmake files.
#
# parameters:
# LIB: the library we are trying to find
# HEADER: the header we are trying to find
# SUFFIX: suffix for the include dir
# VERSION: the operator and version that is given to the pkg-config call (e.g. ">=1.0")
#
# Example: engine_find(SDL2_image SDL_image.h SDL2)
#
macro(engine_find LIB HEADER SUFFIX VERSION)
	string(TOUPPER ${LIB} PREFIX)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_PROCESSOR_ARCH "x64")
	else()
		set(_PROCESSOR_ARCH "x86")
	endif()
	set(_SEARCH_PATHS
		~/Library/Frameworks
		/Library/Frameworks
		/usr/local
		/usr
		/sw # Fink
		/opt/local # DarwinPorts
		/opt/csw # Blastwave
		/opt
	)
	find_package(PkgConfig QUIET)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules(_${PREFIX} QUIET ${LIB}${VERSION})
	endif()
	find_path(${PREFIX}_INCLUDE_DIRS
		NAMES ${HEADER}
		HINTS ENV ${PREFIX}DIR
		PATH_SUFFIXES include include/${SUFFIX} ${SUFFIX}
		PATHS
			${_${PREFIX}_INCLUDE_DIRS}
			${_SEARCH_PATHS}
	)
	find_library(${PREFIX}_LIBRARIES
		NAMES ${LIB}
		HINTS ENV ${PREFIX}DIR
		PATH_SUFFIXES lib64 lib lib/${_PROCESSOR_ARCH}
		PATHS
			${_${PREFIX}_LIBRARY_DIRS}
			${_SEARCH_PATHS}
	)
	unset(_SEARCH_PATHS)
	unset(_PROCESSOR_ARCH)
	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(${PREFIX} REQUIRED_VARS ${PREFIX}_INCLUDE_DIRS ${PREFIX}_LIBRARIES)
	mark_as_advanced(${PREFIX}_INCLUDE_DIRS ${PREFIX}_LIBRARIES)
endmacro()
