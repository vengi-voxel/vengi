set(GAME_BASE_DIR data CACHE STRING "" FORCE)
set(LIBS_DIR ${PROJECT_SOURCE_DIR}/contrib/libs)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(TOOLS_DIR ${ROOT_DIR}/tools/win32 CACHE STRING "" FORCE)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(TOOLS_DIR ${ROOT_DIR}/tools//osx CACHE STRING "" FORCE)
else()
	set(TOOLS_DIR ${ROOT_DIR}/tools//linux CACHE STRING "" FORCE)
endif()

macro(var_global VARIABLES)
	foreach(VAR ${VARIABLES})
		set(${VAR} ${${VAR}} CACHE STRING "" FORCE)
		mark_as_advanced(${VAR})
	endforeach()
endmacro()

# some cross compiling toolchains define this
if(NOT COMMAND find_host_program)
	macro(find_host_program)
		find_program(${ARGN})
	endmacro()
endif()

macro(convert_to_camel_case IN OUT)
	string(REPLACE "_" ";" _list ${IN})
	set(_final "")
	if (_list)
		foreach(_e ${_list})
			string(SUBSTRING ${_e} 0 1 _first_letter)
			string(TOUPPER ${_first_letter} _first_letter)
			string(SUBSTRING ${_e} 1 -1 _remaining)
			set(_final "${_final}${_first_letter}${_remaining}")
		endforeach()
	else()
		string(SUBSTRING ${IN} 0 1 _first_letter)
		string(TOUPPER ${_first_letter} _first_letter)
		string(SUBSTRING ${IN} 1 -1 _remaining)
		set(_final "${_final}${_first_letter}${_remaining}")
	endif()
	set(${OUT} ${_final})
endmacro()

macro(generate_shaders TARGET)
	set(files ${ARGV})
	list(REMOVE_AT files 0)
	find_program(GLSL_VALIDATOR_EXECUTABLE NAMES glslangValidator PATHS ${TOOLS_DIR})
	set(_headers)
	set(GEN_DIR ${CMAKE_BINARY_DIR}/gen-shaders/${TARGET}/)
	set(_template ${ROOT_DIR}/src/tools/shadertool/ShaderTemplate.h.in)
	file(MAKE_DIRECTORY ${GEN_DIR})
	target_include_directories(${TARGET} PUBLIC ${GEN_DIR})
	if (GLSL_VALIDATOR_EXECUTABLE)
		message("${GLSL_VALIDATOR_EXECUTABLE} found - executing in ${ROOT_DIR}/data/${TARGET}/shaders")
		set(_dir ${ROOT_DIR}/data/${TARGET}/shaders)
		if (IS_DIRECTORY ${_dir})
			foreach(_file ${files})
				if (EXISTS ${_dir}/${_file}.frag AND EXISTS ${_dir}/${_file}.vert)
					convert_to_camel_case(${_file} _f)
					set(_shaderfile "${_f}Shader.h")
					set(_shader "${GEN_DIR}${_shaderfile}")
					add_custom_command(
						OUTPUT ${_shader}
						COMMENT "Validate ${_file} and generate ${_shaderfile}"
						COMMAND ${CMAKE_BINARY_DIR}/shadertool ${GLSL_VALIDATOR_EXECUTABLE} ${_file} ${_template} shader shaders/ ${GEN_DIR}
						DEPENDS shadertool ${_dir}/${_file}.frag ${_dir}/${_file}.vert ${_template}
						WORKING_DIRECTORY ${_dir}
					)
					list(APPEND _headers ${_shader})
				endif()
			endforeach()
		endif()
		set(_dir ${ROOT_DIR}/data/shared/shaders)
		if (IS_DIRECTORY ${_dir})
			foreach(_file ${files})
				if (EXISTS ${_dir}/${_file}.frag AND EXISTS ${_dir}/${_file}.vert)
					convert_to_camel_case(${_file} _f)
					set(_shaderfile "${_f}Shader.h")
					set(_shader "${GEN_DIR}${_shaderfile}")
					add_custom_command(
						OUTPUT ${_shader}
						COMMENT "Validate ${_file} and generate ${_shaderfile}"
						COMMAND ${CMAKE_BINARY_DIR}/shadertool ${GLSL_VALIDATOR_EXECUTABLE} ${_file} ${_template} shader shaders/ ${GEN_DIR}
						DEPENDS shadertool ${_dir}/${_file}.frag ${_dir}/${_file}.vert ${_template}
						WORKING_DIRECTORY ${_dir}
					)
					list(APPEND _headers ${_shader})
				endif()
			endforeach()
		endif()

		add_custom_target(GenerateShaderBindings${TARGET}
			DEPENDS ${_headers}
			COMMENT "Generate shader bindings for ${TARGET} in ${GEN_DIR}"
		)
		convert_to_camel_case(${TARGET} _filetarget)
		set(_h ${GEN_DIR}/${_filetarget}Shaders.h)
		file(WRITE ${_h} "#pragma once\n")
		foreach(header_path ${_headers})
			string(REPLACE "${GEN_DIR}" "" header "${header_path}")
			file(APPEND ${_h} "#include \"${header}\"\n")
		endforeach()

		#target_sources(${TARGET} PUBLIC ${_headers} ${_h})
		set_source_files_properties(${_headers} ${_h} PROPERTIES GENERATED TRUE)
		add_dependencies(${TARGET} GenerateShaderBindings${TARGET})
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
		set(LIBS ${${PREFIX}_LIBRARIES})
		if (LIBS)
			# Remove leading spaces
			string(REGEX REPLACE "^[ \t\r\n]+" "" LIBS "${LIBS}" )
			# Remove trailing spaces
			string(REGEX REPLACE "(\ )+$" "" LIBS ${LIBS})
			target_link_libraries(${_ADDLIB_LIB} INTERFACE ${LIBS})
		endif()
		if (${PREFIX}_INCLUDE_DIRS)
			set_property(TARGET ${_ADDLIB_LIB} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${${PREFIX}_INCLUDE_DIRS})
		endif()
	else()
		message(STATUS "Use the bundled lib ${_ADDLIB_LIB}")
		add_library(${_ADDLIB_LIB} STATIC ${_ADDLIB_SRCS})
		target_include_directories(${_ADDLIB_LIB} ${_ADDLIB_PUBLICHEADER} ${LIBS_DIR}/${_ADDLIB_LIB})
		set_target_properties(${_ADDLIB_LIB} PROPERTIES COMPILE_DEFINITIONS "${_ADDLIB_DEFINES}")
		if (NOT MSVC)
			set_target_properties(${_ADDLIB_LIB} PROPERTIES COMPILE_FLAGS "${_ADDLIB_GCCCFLAGS}")
			set_target_properties(${_ADDLIB_LIB} PROPERTIES LINK_FLAGS "${_ADDLIB_GCCLINKERFLAGS}")
		endif()
		set_target_properties(${_ADDLIB_LIB} PROPERTIES FOLDER ${_ADDLIB_LIB})
	endif()
endmacro()

#-------------------------------------------------------------------------------
#   Macros for generating google unit tests.
#-------------------------------------------------------------------------------

set(GOOGLETESTDIR ${CMAKE_CURRENT_LIST_DIR})

#-------------------------------------------------------------------------------
#   gtest_suite_begin(name)
#   Begin defining a unit test suite.
#
macro(gtest_suite_begin name)
	if (UNITTESTS)
		set(options NO_TEMPLATE)
		set(oneValueArgs TEMPLATE)
		set(multiValueArgs)
		cmake_parse_arguments(${name} "${options}" "${oneValueArgs}" "" ${ARGN})

		if (${name}_UNPARSED_ARGUMENTS)
			message(FATAL_ERROR "gtest_suite_begin(): called with invalid args '${${name}_UNPARSED_ARGUMENTS}'")
		endif()
		set_property(GLOBAL PROPERTY ${name}_Sources "")
		set_property(GLOBAL PROPERTY ${name}_Deps "")
	endif()
endmacro()

#-------------------------------------------------------------------------------
#   gtest_suite_files(files)
#   Adds files to a test suite
#
macro(gtest_suite_files name)
	if (UNITTESTS)
		set(ARG_LIST ${ARGV})
		list(REMOVE_AT ARG_LIST 0)
		get_property(list GLOBAL PROPERTY ${name}_Sources)
		foreach(entry ${ARG_LIST})
			list(APPEND list ${CMAKE_CURRENT_SOURCE_DIR}/${entry})
		endforeach()
		set_property(GLOBAL PROPERTY ${name}_Sources ${list})
	endif()
endmacro()

#-------------------------------------------------------------------------------
#   gtest_suite_deps(files)
#   Adds files to a test suite
#
macro(gtest_suite_deps name)
	if (UNITTESTS)
		set(ARG_LIST ${ARGV})
		list(REMOVE_AT ARG_LIST 0)
		get_property(list GLOBAL PROPERTY ${name}_Deps)
		list(APPEND list ${ARG_LIST})
		set_property(GLOBAL PROPERTY ${name}_Deps ${list})
	endif()
endmacro()

#-------------------------------------------------------------------------------
#   gtest_suite_end()
#   End defining a unittest suite
#
macro(gtest_suite_end name)
	if (UNITTESTS)
		project(${name})
		get_property(srcs GLOBAL PROPERTY ${name}_Sources)
		get_property(deps GLOBAL PROPERTY ${name}_Deps)

		if (NOT ${name}_NO_TEMPLATE)
			set(main_path ${CMAKE_CURRENT_BINARY_DIR}/${name}_main.cpp)
			if (${name}_TEMPLATE)
				configure_file(${${name}_TEMPLATE} ${main_path})
			else()
				configure_file(${GOOGLETESTDIR}/main.cpp.in ${main_path})
			endif()
			list(APPEND srcs ${main_path})
		endif()

		add_executable(${name} ${srcs})
		# add googletest lib dependency
		target_link_libraries(${name} gtest ${deps})
		# generate a command line app
		set_target_properties(${name} PROPERTIES FOLDER "tests")

		# add as cmake unit test
		add_test(NAME ${name} COMMAND ${name})

		copy_data_files(${name})
	endif()
endmacro()

#
# set up the binary for the application. This will also set up platform specific stuff for you
#
# Example: engine_add_executable(TARGET SomeTargetName SRCS Source.cpp Main.cpp WINDOWED)
#
macro(engine_add_executable)
	set(_OPTIONS_ARGS WINDOWED)
	set(_ONE_VALUE_ARGS TARGET)
	set(_MULTI_VALUE_ARGS SRCS)

	cmake_parse_arguments(_EXE "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN} )

	if (_EXE_WINDOWED)
		if (WINDOWS)
			add_executable(${_EXE_TARGET} WIN32 ${_EXE_SRCS})
			if (MSVC)
				set_target_properties(${_EXE_TARGET} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
			endif()
		else()
			add_executable(${_EXE_TARGET} ${_EXE_SRCS})
		endif()
	else()
		add_executable(${_EXE_TARGET} ${_EXE_SRCS})
		if (WINDOWS)
			if (MSVC)
				set_target_properties(${_EXE_TARGET} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
			endif()
		endif()
	endif()

	set(RESOURCE_DIRS ${ROOT_DIR}/data/${_EXE_TARGET}/ ${ROOT_DIR}/data/shared/)
	copy_data_files(${_EXE_TARGET})
	# by default, put system related files into the current binary dir on install
	set(SHARE_DIR ".")
	# by default, put data files into the current binary dir on install
	set(GAMES_DIR "${_EXE_TARGET}")
	# by default, put the binary into a subdir with the target name
	set(BIN_DIR "${_EXE_TARGET}")
	set(ICON_DIR ".")

	if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
		set(SHARE_DIR "shared")
		set(GAMES_DIR "${SHARE_DIR}/${_EXE_TARGET}")
		set(ICON_DIR "${SHARE_DIR}/icons")
		set(BIN_DIR "games")
		configure_file(${ROOT_DIR}/contrib/installer/linux/desktop.in ${PROJECT_BINARY_DIR}/${_EXE_TARGET}.desktop)
		install(FILES ${PROJECT_BINARY_DIR}/${_EXE_TARGET}.desktop DESTINATION ${SHARE_DIR}/applications)
	endif()

	set(ICON "${_EXE_TARGET}-icon.png")
	if (EXISTS ${ROOT_DIR}/contrib/${ICON})
		install(FILES ${ROOT_DIR}/contrib/${ICON} DESTINATION ${ICON_DIR} COMPONENT ${_EXE_TARGET})
	endif()

	foreach (dir ${RESOURCE_DIRS})
		if (IS_DIRECTORY ${dir})
			install(DIRECTORY ${dir} DESTINATION ${GAMES_DIR}/ COMPONENT ${_EXE_TARGET})
		endif()
	endforeach()
	install(TARGETS ${_EXE_TARGET} DESTINATION ${BIN_DIR} COMPONENT ${_EXE_TARGET})
endmacro()
