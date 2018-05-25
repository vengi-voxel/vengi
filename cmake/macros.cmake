include(CMakeParseArguments)
include(CheckCXXCompilerFlag)
include(CheckCCompilerFlag)

set(LIBS_DIR ${PROJECT_SOURCE_DIR}/contrib/libs)

if (CMAKE_COMPILER_IS_GNUCC)
	set(USE_GCC TRUE)
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "AppleClang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
	set(USE_CLANG TRUE)
elseif (MSVC)
	set(USE_MSVC TRUE)
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
	set(GEN_DIR ${GENERATE_DIR}/shaders/${TARGET}/)
	set(_template_header ${ROOT_DIR}/src/tools/shadertool/ShaderTemplate.h.in)
	set(_template_cpp ${ROOT_DIR}/src/tools/shadertool/ShaderTemplate.cpp.in)
	set(_template_ub ${ROOT_DIR}/src/tools/shadertool/UniformBufferTemplate.h.in)
	file(MAKE_DIRECTORY ${GEN_DIR})
	target_include_directories(${TARGET} PUBLIC ${GEN_DIR})
	set(_headers)
	set(_sources)
	add_custom_target(UpdateShaders${TARGET})
	file(WRITE ${CMAKE_BINARY_DIR}/UpdateShaderFile${TARGET}.cmake "configure_file(\${SRC} \${DST} @ONLY)")
	foreach (_file ${files})
		set(_shaders)
		set(_dir ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
		if (EXISTS ${_dir}/${_file}.frag AND EXISTS ${_dir}/${_file}.vert)
			list(APPEND _shaders ${_dir}/${_file}.frag ${_dir}/${_file}.vert)
			if (EXISTS ${_dir}/${_file}.geom)
				list(APPEND _shaders ${_dir}/${_file}.geom)
			endif()
		endif()
		if (EXISTS ${_dir}/${_file}.comp)
			list(APPEND _shaders ${_dir}/${_file}.comp)
		endif()
		if (_shaders)
			convert_to_camel_case(${_file} _f)
			set(_shaderheaderpath "${GEN_DIR}${_f}Shader.h")
			set(_shadersourcepath "${GEN_DIR}${_f}Shader.cpp")
			add_custom_command(
				OUTPUT ${_shaderheaderpath}.in ${_shadersourcepath}.in
				IMPLICIT_DEPENDS C ${_shaders}
				COMMENT "Validate ${_file}"
				COMMAND ${CMAKE_BINARY_DIR}/shadertool --glslang ${CMAKE_BINARY_DIR}/glslangValidator -I ${_dir} -I ${PROJECT_SOURCE_DIR}/src/modules/video/shaders --postfix .in --shader ${_dir}/${_file} --headertemplate ${_template_header} --sourcetemplate ${_template_cpp} --buffertemplate ${_template_ub} --sourcedir ${GEN_DIR}
				DEPENDS shadertool ${_shaders} ${_template_header} ${_template_cpp} ${_template_ub}
			)
			list(APPEND _headers ${_shaderheaderpath})
			list(APPEND _sources ${_shadersourcepath})
			add_custom_command(
				OUTPUT ${_shaderheaderpath}
				COMMAND ${CMAKE_COMMAND} -D SRC=${_shaderheaderpath}.in -D DST=${_shaderheaderpath} -P ${CMAKE_BINARY_DIR}/UpdateShaderFile${TARGET}.cmake
				DEPENDS ${_shaderheaderpath}.in
			)
			add_custom_command(
				OUTPUT ${_shadersourcepath}
				COMMAND ${CMAKE_COMMAND} -D SRC=${_shadersourcepath}.in -D DST=${_shadersourcepath} -P ${CMAKE_BINARY_DIR}/UpdateShaderFile${TARGET}.cmake
				DEPENDS ${_shadersourcepath}.in
			)
		else()
			message(FATAL_ERROR "Could not find any shader files for ${_file} and target '${TARGET}'")
		endif()
	endforeach()

	convert_to_camel_case(${TARGET} _filetarget)
	set(_shadersheader ${GEN_DIR}/${_filetarget}Shaders.h)
	file(WRITE ${_shadersheader}.in "#pragma once\n")
	foreach (header_path ${_headers})
		string(REPLACE "${GEN_DIR}" "" header "${header_path}")
		file(APPEND ${_shadersheader}.in "#include \"${header}\"\n")
	endforeach()
	add_custom_target(GenerateShaderBindings${TARGET}
		DEPENDS ${_headers}
		COMMENT "Generate shader bindings for ${TARGET} in ${GEN_DIR}"
	)
	set_source_files_properties(${_headers} PROPERTIES GENERATED TRUE)
	set_source_files_properties(${_sources} PROPERTIES GENERATED TRUE)
	set_source_files_properties(${_shadersheader} PROPERTIES GENERATED TRUE)
	target_sources(${TARGET} PRIVATE ${_headers} ${_sources} ${_shadersheader})
	add_custom_target(GenerateShaderHeader${TARGET} ${CMAKE_COMMAND} -D SRC=${_shadersheader}.in -D DST=${_shadersheader} -P ${CMAKE_BINARY_DIR}/UpdateShaderFile${TARGET}.cmake)
	add_dependencies(${TARGET} GenerateShaderHeader${TARGET} UpdateShaders${TARGET})
	add_dependencies(GenerateShaderHeader${TARGET} GenerateShaderBindings${TARGET})
	add_dependencies(codegen GenerateShaderHeader${TARGET} UpdateShaders${TARGET})
endmacro()

macro(generate_compute_shaders TARGET)
	set(files ${ARGV})
	list(REMOVE_AT files 0)
	set(GEN_DIR ${GENERATE_DIR}/compute-shaders/${TARGET}/)
	set(_template ${ROOT_DIR}/src/tools/computeshadertool/ComputeShaderTemplate.h.in)
	file(MAKE_DIRECTORY ${GEN_DIR})
	target_include_directories(${TARGET} PUBLIC ${GEN_DIR})
	set(_headers)
	add_custom_target(UpdateComputeShaders${TARGET})
	file(WRITE ${CMAKE_BINARY_DIR}/GenerateComputeShaderHeader${TARGET}.cmake "configure_file(\${SRC} \${DST} @ONLY)")
	foreach (_file ${files})
		set(_shaders)
		set(_dir ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
		if (EXISTS ${_dir}/${_file}.cl)
			list(APPEND _shaders ${_dir}/${_file}.cl)
		endif()
		if (_shaders)
			convert_to_camel_case(${_file} _f)
			set(_shaderfile "${_f}Shader.h")
			set(_shader "${GEN_DIR}${_shaderfile}")
			add_custom_command(
				OUTPUT ${_shader}.in
				IMPLICIT_DEPENDS C ${_shaders}
				COMMENT "Validate ${_file} and generate ${_shaderfile}"
				COMMAND ${CMAKE_BINARY_DIR}/computeshadertool --shader ${_dir}/${_file} -I ${_dir} -I ${PROJECT_SOURCE_DIR}/src/modules/compute/shaders --postfix .in --shadertemplate ${_template} --sourcedir ${GEN_DIR}
				DEPENDS computeshadertool ${_shaders} ${_template}
				VERBATIM
			)
			list(APPEND _headers ${_shader})
			add_custom_command(
				OUTPUT ${_shader}
				COMMAND ${CMAKE_COMMAND} -D SRC=${_shader}.in -D DST=${_shader} -P ${CMAKE_BINARY_DIR}/GenerateComputeShaderHeader${TARGET}.cmake
				DEPENDS ${_shader}.in
			)
		else()
			message(FATAL_ERROR "Could not find any shader files for ${_file} and target '${TARGET}'")
		endif()
	endforeach()

	convert_to_camel_case(${TARGET} _filetarget)
	set(_h ${GEN_DIR}/${_filetarget}Shaders.h)
	file(WRITE ${_h}.in "#pragma once\n")
	foreach (header_path ${_headers})
		string(REPLACE "${GEN_DIR}" "" header "${header_path}")
		file(APPEND ${_h}.in "#include \"${header}\"\n")
	endforeach()
	add_custom_target(GenerateComputeShaderBindings${TARGET}
		DEPENDS ${_headers}
		COMMENT "Generate shader bindings for ${TARGET} in ${GEN_DIR}"
	)
	set_source_files_properties(${_headers} ${_h} PROPERTIES GENERATED TRUE)
	add_custom_target(GenerateComputeShaderHeader${TARGET} ${CMAKE_COMMAND} -D SRC=${_h}.in -D DST=${_h} -P ${CMAKE_BINARY_DIR}/GenerateComputeShaderHeader${TARGET}.cmake)
	add_dependencies(${TARGET} GenerateComputeShaderHeader${TARGET} UpdateComputeShaders${TARGET})
	add_dependencies(GenerateComputeShaderHeader${TARGET} GenerateComputeShaderBindings${TARGET})
	add_dependencies(codegen GenerateComputeShaderHeader${TARGET} UpdateComputeShaders${TARGET})
endmacro()

macro(generate_db_models TARGET INPUT OUTPUT)
	set(GEN_DIR ${GENERATE_DIR}/dbmodels/${TARGET}/)
	file(MAKE_DIRECTORY ${GEN_DIR})
	target_include_directories(${TARGET} PUBLIC ${GEN_DIR})
	add_custom_command(
		OUTPUT ${GEN_DIR}${OUTPUT}
		COMMENT "Generate ${OUTPUT}"
		COMMAND ${CMAKE_BINARY_DIR}/databasetool --tablefile ${INPUT} --outfile ${GEN_DIR}${OUTPUT}
		DEPENDS databasetool ${INPUT}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	)
	add_custom_target(GenerateDatabaseModelBindings${TARGET}
		DEPENDS ${GEN_DIR}${OUTPUT}
		COMMENT "Generate database model bindings for ${TARGET} in ${GEN_DIR}"
	)
	set_source_files_properties(${GEN_DIR}${OUTPUT} PROPERTIES GENERATED TRUE)
	add_dependencies(${TARGET} GenerateDatabaseModelBindings${TARGET})
	add_dependencies(codegen GenerateDatabaseModelBindings${TARGET})
endmacro()

macro(generate_protocol TARGET)
	set(files ${ARGV})
	list(REMOVE_AT files 0)
	set(GEN_DIR ${GENERATE_DIR}/protocol/${TARGET}/)
	file(MAKE_DIRECTORY ${GEN_DIR})
	target_include_directories(${TARGET} PUBLIC ${GEN_DIR})
	set(_headers)
	foreach (_file ${files})
		get_filename_component(_basefilename ${_file} NAME_WE)
		set(HEADER "${_basefilename}_generated.h")
		set(DEFINITION ${_file})
		list(APPEND _headers ${GEN_DIR}${HEADER})
		add_custom_command(
			OUTPUT ${GEN_DIR}${HEADER}
			COMMAND ${CMAKE_BINARY_DIR}/flatc -c -I ${CMAKE_CURRENT_SOURCE_DIR}/../attrib/definitions --scoped-enums -o ${GEN_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/definitions/${DEFINITION}
			DEPENDS flatc ${CMAKE_CURRENT_SOURCE_DIR}/definitions/${DEFINITION}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			COMMENT "Generating source code for ${DEFINITION}"
		)
		set_source_files_properties(${GEN_DIR}/${HEADER} PROPERTIES GENERATED TRUE)
	endforeach()

	add_custom_target(GenerateNetworkMessages${TARGET}
		DEPENDS ${_headers}
		COMMENT "Generate network messages in ${GEN_DIR}"
	)
	add_dependencies(${TARGET} GenerateNetworkMessages${TARGET})
	add_dependencies(codegen GenerateNetworkMessages${TARGET})
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
		else()
			set(${PREFIX}_INCLUDE_DIRS ${PKG_PREFIX}_INCLUDE_DIR)
		endif()
		if (${PREFIX}_LIBRARIES ${PKG_PREFIX}_LIBRARIES)
			set(${PREFIX}_LIBRARIES ${PKG_PREFIX}_LIBRARIES)
		else()
			set(${PREFIX}_LIBRARIES ${PKG_PREFIX}_LIBRARY)
		endif()
		set(${PREFIX}_FOUND ${PKG_PREFIX}_FOUND)
		message(STATUS "find_package ${FIND_PACKAGE_NAME} for ${_ADDLIB_LIB}")
	endif()
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
		if (USE_GCC OR USE_CLANG)
			message(STATUS "additional lib cflags: ${_ADDLIB_GCCCFLAGS}")
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

		if (NOT ${name}_NO_TEMPLATE)
			set(main_path ${CMAKE_CURRENT_BINARY_DIR}/${name}_main.cpp)
			if (${name}_TEMPLATE)
				configure_file(${${name}_TEMPLATE} ${main_path})
			else()
				configure_file(${GOOGLETESTDIR}/main.cpp.in ${main_path})
			endif()
			add_executable(${name} ${main_path})
		else()
			add_executable(${name})
		endif()

		# add googletest lib dependency
		find_package(GTest)
		if (GTEST_FOUND)
			target_include_directories(${name} PRIVATE ${GTEST_INCLUDE_DIRS})
			target_link_libraries(${name} ${GTEST_LIBRARIES})
		else()
			target_link_libraries(${name} gtest)
		endif()

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

		target_sources(${name} PRIVATE ${srcs})

		get_property(models GLOBAL PROPERTY ${name}_Models)
		foreach(entry ${models})
			string(REPLACE ":" ";" inout ${entry})
			list(GET inout 0 in)
			list(GET inout 1 out)
			generate_db_models(${name} ${in} ${out})
		endforeach()
		target_link_libraries(${name} ${deps})
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
# TARGET:    the target name (binary name)
# SRCS:      the source files for this target
# WINDOWED:  this is needed to indicate whether the application should e.g. spawn a console on windows
# NOINSTALL: means that the binary and data files are not put into the final installation folder
#            this can e.g. be useful for stuff like code generators that are only needed during build
#            time.
#
macro(engine_add_executable)
	set(_OPTIONS_ARGS WINDOWED NOINSTALL)
	set(_ONE_VALUE_ARGS TARGET)
	set(_MULTI_VALUE_ARGS SRCS)

	cmake_parse_arguments(_EXE "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN} )

	set(_EXE_CATEGORIES "Game")

	if (_EXE_WINDOWED)
		set(_EXE_TERMINAL "false")
		if (WINDOWS)
			add_executable(${_EXE_TARGET} WIN32 ${_EXE_SRCS})
			if (USE_MSVC)
				set_target_properties(${_EXE_TARGET} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
			endif()
		else()
			add_executable(${_EXE_TARGET} ${_EXE_SRCS})
		endif()
	else()
		set(_EXE_TERMINAL "true")
		add_executable(${_EXE_TARGET} ${_EXE_SRCS})
		if (WINDOWS)
			if (USE_MSVC)
				set_target_properties(${_EXE_TARGET} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
			endif()
		endif()
	endif()

	set(RESOURCE_DIRS ${ROOT_DIR}/${GAME_BASE_DIR}/${_EXE_TARGET}/ ${ROOT_DIR}/${GAME_BASE_DIR}/shared/)
	copy_data_files(${_EXE_TARGET})
	# by default, put system related files into the current binary dir on install
	set(SHARE_DIR ".")
	# by default, put data files into the current binary dir on install
	set(GAMES_DIR "${_EXE_TARGET}")
	# by default, put the binary into a subdir with the target name
	set(BIN_DIR "${_EXE_TARGET}")
	set(ICON_DIR ".")

	if (SANITIZER_THREADS AND NOT ${_EXE_TARGET} STREQUAL "databasetool" AND NOT ${_EXE_TARGET} STREQUAL "shadertool" AND NOT ${_EXE_TARGET} STREQUAL "uitool")
		set_target_properties(${_EXE_TARGET} PROPERTIES COMPILE_FLAGS "${SANITIZE_THREAD_FLAG}")
		set_target_properties(${_EXE_TARGET} PROPERTIES LINK_FLAGS "${SANITIZE_THREAD_FLAG}")
	endif()

	if (NOT _EXE_NOINSTALL)
		set(ICON "${_EXE_TARGET}-icon.png")
		if (EXISTS ${ROOT_DIR}/contrib/${ICON})
			install(FILES ${ROOT_DIR}/contrib/${ICON} DESTINATION ${ICON_DIR} COMPONENT ${_EXE_TARGET})
		endif()

		if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
			set(SHARE_DIR "share")
			set(GAMES_DIR "${SHARE_DIR}/${_EXE_TARGET}")
			set(ICON_DIR "${SHARE_DIR}/icons")
			set(BIN_DIR "games")
			configure_file(${ROOT_DIR}/contrib/installer/linux/desktop.in ${PROJECT_BINARY_DIR}/${_EXE_TARGET}.desktop)
			install(FILES ${PROJECT_BINARY_DIR}/${_EXE_TARGET}.desktop DESTINATION ${SHARE_DIR}/applications)
		endif()

		foreach (dir ${RESOURCE_DIRS})
			if (IS_DIRECTORY ${dir})
				install(DIRECTORY ${dir} DESTINATION ${GAMES_DIR}/ COMPONENT ${_EXE_TARGET})
			endif()
		endforeach()
		install(TARGETS ${_EXE_TARGET} DESTINATION ${BIN_DIR} COMPONENT ${_EXE_TARGET})
	endif()
endmacro()

macro(engine_target_link_libraries)
	set(_OPTIONS_ARGS)
	set(_ONE_VALUE_ARGS TARGET)
	set(_MULTI_VALUE_ARGS DEPENDENCIES)

	cmake_parse_arguments(_LIBS "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN} )

	target_link_libraries(${_LIBS_TARGET} ${_LIBS_DEPENDENCIES})
endmacro()
