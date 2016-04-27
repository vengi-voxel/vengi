if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(TOOLS_DIR ${FIPS_PROJECT_DIR}/tools/win32 CACHE STRING "" FORCE)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(TOOLS_DIR ${FIPS_PROJECT_DIR}/tools/osx CACHE STRING "" FORCE)
else()
	set(TOOLS_DIR ${FIPS_PROJECT_DIR}/tools/linux CACHE STRING "" FORCE)
endif()

macro(var_global VARIABLES)
	foreach(VAR ${VARIABLES})
		set(${VAR} ${${VAR}} CACHE STRING "" FORCE)
		mark_as_advanced(${VAR})
	endforeach()
endmacro()

macro(check_glsl_files TARGET)
	set(files ${ARGV})
	list(REMOVE_AT files 0)
	find_program(GLSL_VALIDATOR_EXECUTABLE NAMES glslangValidator PATHS ${TOOLS_DIR})
	if (GLSL_VALIDATOR_EXECUTABLE)
		message("${GLSL_VALIDATOR_EXECUTABLE} found - executing in ${FIPS_PROJECT_DIR}/data/${TARGET}/shaders")
		foreach(_file ${files})
			add_custom_target(
				${TARGET}_${_file}_shader_validation
				COMMENT "Validate ${_file}"
				COMMAND ${FIPS_DEPLOY_DIR}/${CMAKE_PROJECT_NAME}/${FIPS_CONFIG}/shadertool ${GLSL_VALIDATOR_EXECUTABLE} ${_file}
				DEPENDS shadertool
				WORKING_DIRECTORY ${FIPS_DEPLOY_DIR}/${CMAKE_PROJECT_NAME}/${FIPS_CONFIG}/shaders
			)
			add_dependencies(${TARGET} shadertool ${TARGET}_${_file}_shader_validation)
		endforeach()
	else()
		message(WARNING "No ${GLSL_VALIDATOR_EXECUTABLE} found at ${TOOLS_DIR}")
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
#          (this only works for pkg-config)
#
# Example: engine_find(SDL2_image SDL_image.h SDL2 "")
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
		pkg_check_modules(_${PREFIX} "${LIB}${VERSION}")
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
		NAMES ${LIB} ${PREFIX} ${_${PREFIX}_LIBRARIES}
		HINTS ENV ${PREFIX}DIR
		PATH_SUFFIXES lib64 lib lib/${_PROCESSOR_ARCH}
		PATHS
			${_${PREFIX}_LIBRARY_DIRS}
			${_SEARCH_PATHS}
	)
	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(${LIB} FOUND_VAR ${PREFIX}_FOUND REQUIRED_VARS ${PREFIX}_INCLUDE_DIRS ${PREFIX}_LIBRARIES)
	mark_as_advanced(${PREFIX}_INCLUDE_DIRS ${PREFIX}_LIBRARIES ${PREFIX}_FOUND)
	var_global(${PREFIX}_INCLUDE_DIRS ${PREFIX}_LIBRARIES ${PREFIX}_FOUND)
	unset(PREFIX)
	unset(_SEARCH_PATHS)
	unset(_PROCESSOR_ARCH)
endmacro()

macro(engine_find_header_only LIB HEADER SUFFIX VERSION)
	string(TOUPPER ${LIB} PREFIX)
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
		pkg_check_modules(_${PREFIX} "${LIB}${VERSION}")
	endif()
	find_path(${PREFIX}_INCLUDE_DIRS
		NAMES ${HEADER}
		HINTS ENV ${PREFIX}DIR
		PATH_SUFFIXES include include/${SUFFIX} ${SUFFIX}
		PATHS
			${_${PREFIX}_INCLUDE_DIRS}
			${_SEARCH_PATHS}
	)
	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(${LIB} FOUND_VAR ${PREFIX}_FOUND REQUIRED_VARS ${PREFIX}_INCLUDE_DIRS)
	mark_as_advanced(${PREFIX}_INCLUDE_DIRS ${PREFIX}_FOUND)
	var_global(${PREFIX}_INCLUDE_DIRS ${PREFIX}_FOUND)
	unset(PREFIX)
	unset(_SEARCH_PATHS)
endmacro()

#
# Add external dependency. It will trigger a find_package and use the system wide install if found
#
# parameters:
# PUBLICHEADER: optional
# LIB: the name of the lib. Must match the FindXXX.cmake module and the pkg-config name of the lib
# GCCCFLAGS: optional
# GCCLINKERFLAGS: optional
# SRCS: the list of source files for the bundled lib
# DEFINES: a list of defines (without -D or /D)
#
macro(engine_add_library)
	set(_OPTIONS_ARGS)
	set(_ONE_VALUE_ARGS LIB GCCCFLAGS LINKERFLAGS PUBLICHEADER)
	set(_MULTI_VALUE_ARGS SRCS DEFINES)

	cmake_parse_arguments(_ADDLIB "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN} )

	if (NOT _ADDLIB_LIB)
		message(FATAL_ERROR "engine_add_library requires the LIB argument")
	endif()
	if (NOT _ADDLIB_SRCS)
		message(FATAL_ERROR "engine_add_library requires the SRCS argument")
	endif()
	if (NOT _ADDLIB_PUBLICHEADER)
		set(_ADDLIB_PUBLICHEADER PUBLIC)
	endif()

	find_package(${_ADDLIB_LIB})
	string(TOUPPER ${_ADDLIB_LIB} PREFIX)
	var_global(${PREFIX}_INCLUDE_DIRS ${PREFIX}_LIBRARIES ${PREFIX}_FOUND)
	if (${PREFIX}_FOUND)
		add_library(${_ADDLIB_LIB} INTERFACE)
		target_link_libraries(${_ADDLIB_LIB} INTERFACE ${${PREFIX}_LIBRARIES})
		if (${PREFIX}_INCLUDE_DIRS)
			set_property(TARGET ${_ADDLIB_LIB} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${${PREFIX}_INCLUDE_DIRS})
		endif()
	else()
		message(WARNING "Use the bundled lib ${_ADDLIB_LIB}")
		add_library(${_ADDLIB_LIB} STATIC ${_ADDLIB_SRCS})
		target_include_directories(${_ADDLIB_LIB} ${_ADDLIB_PUBLICHEADER} ${PROJECT_SOURCE_DIR}/src/libs/${_ADDLIB_LIB})
		set_target_properties(${_ADDLIB_LIB} PROPERTIES COMPILE_DEFINITIONS "${_ADDLIB_DEFINES}")
		if (NOT MSVC)
			set_target_properties(${_ADDLIB_LIB} PROPERTIES COMPILE_FLAGS "${_ADDLIB_GCCCFLAGS}")
			set_target_properties(${_ADDLIB_LIB} PROPERTIES LINK_FLAGS "${_ADDLIB_GCCLINKERFLAGS}")
		endif()
		set_target_properties(${_ADDLIB_LIB} PROPERTIES FOLDER ${_ADDLIB_LIB})
	endif()
endmacro()
