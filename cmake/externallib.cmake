set(LIBS_DIR ${PROJECT_SOURCE_DIR}/contrib/libs)

macro(var_global VARIABLES)
	foreach(VAR ${VARIABLES})
		set(${VAR} ${${VAR}} CACHE STRING "" FORCE)
		mark_as_advanced(${VAR})
	endforeach()
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
		$ENV{VCPKG_ROOT}/installed/${_PROCESSOR_ARCH}-windows
		C:/Tools/vcpkg/installed/${_PROCESSOR_ARCH}-windows
		C:/vcpkg/installed/${_PROCESSOR_ARCH}-windows
	)
	find_package(PkgConfig QUIET)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules(_${PREFIX} "${LIB}${VERSION}")
	endif()
	if (NOT ${PREFIX}_FOUND)
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
	else()
		set(${PREFIX}_INCLUDE_DIRS ${_${PREFIX}_INCLUDE_DIRS})
		list(APPEND ${PREFIX}_INCLUDE_DIRS ${_${PREFIX}_INCLUDEDIR})
		set(${PREFIX}_LIBRARIES ${_${PREFIX}_LIBRARIES})
		list(APPEND ${PREFIX}_LIBRARIES ${_${PREFIX}_LIBRARY})
	endif()
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
	set(_OPTIONS_ARGS UNITY)
	set(_ONE_VALUE_ARGS LIB PACKAGE GCCCFLAGS LINKERFLAGS PUBLICHEADER)
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

	if (_ADDLIB_FIND_PACKAGE)
		set(FIND_PACKAGE_NAME ${_ADDLIB_FIND_PACKAGE})
	else()
		set(FIND_PACKAGE_NAME ${_ADDLIB_LIB})
	endif()

	string(TOUPPER ${_ADDLIB_LIB} PREFIX)
	if (NOT ${PREFIX}_LOCAL)
		find_package(${FIND_PACKAGE_NAME})
	endif()
	# now convert it again - looks like find_package exports PREFIX in some versions of cmake, too
	string(TOUPPER ${_ADDLIB_LIB} PREFIX)
	string(TOUPPER ${FIND_PACKAGE_NAME} PKG_PREFIX)
	if (NOT ${PREFIX} STREQUAL ${PKG_PREFIX})
		if (${PKG_PREFIX}_INCLUDE_DIRS)
			set(${PREFIX}_INCLUDE_DIRS ${PKG_PREFIX}_INCLUDE_DIRS)
		elseif(${PKG_PREFIX}_INCLUDE_DIR)
			set(${PREFIX}_INCLUDE_DIRS ${PKG_PREFIX}_INCLUDE_DIR)
		else()
			set(${PREFIX}_INCLUDE_DIRS ${PKG_PREFIX}_INCLUDEDIR)
		endif()
		if (NOT ${PREFIX}_LIBRARIES AND ${PKG_PREFIX}_LIBRARIES)
			set(${PREFIX}_LIBRARIES ${PKG_PREFIX}_LIBRARIES)
		elseif(NOT ${PREFIX}_LIBRARIES AND ${PKG_PREFIX}_LIBRARY)
			set(${PREFIX}_LIBRARIES ${PKG_PREFIX}_LIBRARY)
		else()
			message(WARN "Could not find libs for ${PREFIX}")
		endif()
		message(STATUS "${PKG_PREFIX} => ${${PREFIX}_FOUND}")
		set(${PREFIX}_FOUND ${PKG_PREFIX}_FOUND)
	endif()
	var_global(${PREFIX}_INCLUDE_DIRS ${PREFIX}_LIBRARIES ${PREFIX}_FOUND)
	if (${PREFIX}_FOUND)
		message(STATUS "Found system wide package ${_ADDLIB_LIB}")
		add_library(${_ADDLIB_LIB} INTERFACE)
		if (${PREFIX}_INCLUDE_DIRS)
			target_include_directories(${_ADDLIB_LIB} INTERFACE ${${PREFIX}_INCLUDE_DIRS})
			message(STATUS "${_ADDLIB_LIB}: ${PREFIX}_INCLUDE_DIRS: ${${PREFIX}_INCLUDE_DIRS}")
		endif()
		if (${PREFIX}_LIBRARIES)
			target_link_libraries(${_ADDLIB_LIB} INTERFACE ${${PREFIX}_LIBRARIES})
			message(STATUS "${_ADDLIB_LIB}: ${PREFIX}_LIBRARIES: ${${PREFIX}_LIBRARIES} ${_ADDLIB_ADDITIONAL_LIBS}")
		endif()
	else()
		message(STATUS "Use the bundled lib ${_ADDLIB_LIB}")
		if (_ADDLIB_UNITY AND NOT CMAKE_CROSSCOMPILING AND NOT DISABLE_UNITY)
			set(UNITY_SRC_CPP ${CMAKE_CURRENT_BINARY_DIR}/${_ADDLIB_LIB}_unity.cpp)
			file(WRITE ${UNITY_SRC_CPP}.in "/** autogenerated */\n")
			set(UNITY_SRC_C ${CMAKE_CURRENT_BINARY_DIR}/${_ADDLIB_LIB}_unity.c)
			file(WRITE ${UNITY_SRC_C}.in "/** autogenerated */\n")
			set(unity_srcs)
			list(APPEND unity_srcs ${UNITY_SRC_CPP} ${UNITY_SRC_C})
			foreach(SRC ${_ADDLIB_SRCS})
				get_filename_component(extension ${SRC} EXT)
				if ("${extension}" STREQUAL ".cpp")
					file(APPEND ${UNITY_SRC_CPP}.in "#include \"${SRC}\"\n")
					continue()
				endif()
				if ("${extension}" STREQUAL ".c")
					file(APPEND ${UNITY_SRC_C}.in "#include \"${SRC}\"\n")
					continue()
				endif()
				list(APPEND unity_srcs ${SRC})
			endforeach()
			configure_file(${UNITY_SRC_CPP}.in ${UNITY_SRC_CPP})
			configure_file(${UNITY_SRC_C}.in ${UNITY_SRC_C})
			add_library(${_ADDLIB_LIB} STATIC ${unity_srcs})
		else()
			add_library(${_ADDLIB_LIB} STATIC ${_ADDLIB_SRCS})
		endif()
		target_include_directories(${_ADDLIB_LIB} ${_ADDLIB_PUBLICHEADER} ${LIBS_DIR}/${_ADDLIB_LIB})
		set_target_properties(${_ADDLIB_LIB} PROPERTIES COMPILE_DEFINITIONS "${_ADDLIB_DEFINES}")

		if (USE_GCC OR USE_CLANG)
			if (_ADDLIB_GCCCFLAGS)
				message(STATUS "additional lib cflags: ${_ADDLIB_GCCCFLAGS}")
			endif()
			if(UNIX)
				set_target_properties(${_ADDLIB_LIB} PROPERTIES COMPILE_FLAGS "${_ADDLIB_GCCCFLAGS} -fPIC")
			else()
				set_target_properties(${_ADDLIB_LIB} PROPERTIES COMPILE_FLAGS "${_ADDLIB_GCCCFLAGS}")
			endif()
			set_target_properties(${_ADDLIB_LIB} PROPERTIES LINK_FLAGS "${_ADDLIB_GCCLINKERFLAGS}")
		endif()
		set_target_properties(${_ADDLIB_LIB} PROPERTIES FOLDER ${_ADDLIB_LIB})
	endif()
endmacro()
