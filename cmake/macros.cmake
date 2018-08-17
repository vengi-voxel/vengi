include(CMakeParseArguments)
include(CheckCXXCompilerFlag)
include(CheckCCompilerFlag)
include(GNUInstallDirs)

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

macro(generate_unity_sources)
	set(_OPTIONS_ARGS SOURCES EXECUTABLE LIBRARY WINDOWED)
	set(_ONE_VALUE_ARGS TARGET UNITY_SRC)
	set(_MULTI_VALUE_ARGS SRCS)

	cmake_parse_arguments(_UNITY "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN} )

	if (NOT _UNITY_TARGET)
		message(FATAL_ERROR "generate_unity_sources requires the TARGET argument")
	endif()
	if (NOT _UNITY_SRCS)
		message(FATAL_ERROR "generate_unity_sources requires the SRCS argument")
	endif()
	if (NOT _UNITY_UNITY_SRC)
		set(_UNITY_UNITY_SRC "${CMAKE_CURRENT_BINARY_DIR}/${_UNITY_TARGET}_unity.cpp")
	endif()

	set(TARGET ${_UNITY_TARGET})
	set(UNITY_SRC ${_UNITY_UNITY_SRC})
	set(SRCS ${_UNITY_SRCS})

	get_property(NOUNITY GLOBAL PROPERTY ${TARGET}_NOUNITY)
	if (NOUNITY)
		if (_UNITY_SOURCES)
			target_sources(${TARGET} PRIVATE ${SRCS})
		elseif (_UNITY_EXECUTABLE)
			if (_UNITY_WINDOWED)
				if (WINDOWS)
					add_executable(${TARGET} WIN32 ${SRCS})
					if (USE_MSVC)
						set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
					endif()
				else()
					add_executable(${TARGET} ${SRCS})
				endif()
			else()
				add_executable(${TARGET} ${SRCS})
				if (WINDOWS)
					if (USE_MSVC)
						set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
					endif()
				endif()
			endif()
		elseif (_UNITY_LIBRARY)
			add_library(${TARGET} ${SRCS})
		endif()
	else()
		set(unity_srcs)
		list(APPEND unity_srcs ${UNITY_SRC})
		add_custom_command(
			OUTPUT ${UNITY_SRC}
			COMMAND ${CMAKE_COMMAND} -D "SRCS=\"${SRCS}\"" -D UNITY_SRC="${UNITY_SRC}" -D DIR="${CMAKE_CURRENT_SOURCE_DIR}" -P "${ROOT_DIR}/cmake/GenerateUnity.cmake"
			DEPENDS ${SRCS}
			COMMENT "Generate unity sources for ${TARGET}"
		)
		set_source_files_properties(${UNITY_SRC} ${UNITY_SRC}.in PROPERTIES GENERATED TRUE)
		foreach(SRC ${SRCS})
			get_filename_component(extension ${SRC} EXT)
			if (NOT "${extension}" STREQUAL ".cpp")
				list(APPEND unity_srcs ${SRC})
				continue()
			endif()
		endforeach()
		if (_UNITY_SOURCES)
			target_sources(${TARGET} PRIVATE ${unity_srcs})
		elseif (_UNITY_EXECUTABLE)
			if (_UNITY_WINDOWED)
				if (WINDOWS)
					add_executable(${TARGET} WIN32 ${unity_srcs})
					if (USE_MSVC)
						set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
					endif()
				else()
					add_executable(${TARGET} ${unity_srcs})
				endif()
			else()
				add_executable(${TARGET} ${unity_srcs})
				if (WINDOWS)
					if (USE_MSVC)
						set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
					endif()
				endif()
			endif()
		elseif (_UNITY_LIBRARY)
			add_library(${TARGET} ${unity_srcs})
		endif()
	endif()
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
	if (NOT DEFINED video_SOURCE_DIR)
		message(FATAL_ERROR "video project not found")
	endif()
	set(SHADERTOOL_INCLUDE_DIRS)
	list(APPEND SHADERTOOL_INCLUDE_DIRS "${video_SOURCE_DIR}/shaders")
	get_property(DEPENDENCIES GLOBAL PROPERTY ${TARGET}_DEPENDENCIES)
	foreach (D ${DEPENDENCIES})
		if (NOT DEFINED ${D}_SOURCE_DIR)
			continue()
		endif()
		if (EXISTS ${${D}_SOURCE_DIR}/shaders)
			list(APPEND SHADERTOOL_INCLUDE_DIRS "${${D}_SOURCE_DIR}/shaders")
		endif()
	endforeach()
	list(REMOVE_DUPLICATES SHADERTOOL_INCLUDE_DIRS)
	set(SHADERTOOL_INCLUDE_DIRS_PARAM)
	foreach (IDIR ${SHADERTOOL_INCLUDE_DIRS})
		list(APPEND SHADERTOOL_INCLUDE_DIRS_PARAM "-I")
		list(APPEND SHADERTOOL_INCLUDE_DIRS_PARAM "${IDIR}")
	endforeach()
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
			# TODO We have to add the shader/ dirs of all dependencies to the include path
			add_custom_command(
				OUTPUT ${_shaderheaderpath}.in ${_shadersourcepath}.in
				IMPLICIT_DEPENDS C ${_shaders}
				COMMENT "Validate ${_file}"
				COMMAND $<TARGET_FILE:shadertool> --glslang ${CMAKE_BINARY_DIR}/glslangValidator -I ${_dir} ${SHADERTOOL_INCLUDE_DIRS_PARAM} --postfix .in --shader ${_dir}/${_file} --headertemplate ${_template_header} --sourcetemplate ${_template_cpp} --buffertemplate ${_template_ub} --sourcedir ${GEN_DIR}
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
	generate_unity_sources(SOURCES TARGET ${TARGET} SRCS ${_sources} UNITY_SRC "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_shaders_unity.cpp")
	target_sources(${TARGET} PRIVATE ${_headers} ${_shadersheader})

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
	if (NOT DEFINED compute_SOURCE_DIR)
		message(FATAL_ERROR "compute project not found")
	endif()
	set(SHADERTOOL_INCLUDE_DIRS)
	list(APPEND SHADERTOOL_INCLUDE_DIRS "${compute_SOURCE_DIR}/shaders")
	get_property(DEPENDENCIES GLOBAL PROPERTY ${TARGET}_DEPENDENCIES)
	foreach (D ${DEPENDENCIES})
		if (NOT DEFINED ${D}_SOURCE_DIR)
			continue()
		endif()
		if (EXISTS ${${D}_SOURCE_DIR}/shaders)
			list(APPEND SHADERTOOL_INCLUDE_DIRS "${${D}_SOURCE_DIR}/shaders")
		endif()
	endforeach()
	list(REMOVE_DUPLICATES SHADERTOOL_INCLUDE_DIRS)
	set(SHADERTOOL_INCLUDE_DIRS_PARAM)
	foreach (IDIR ${SHADERTOOL_INCLUDE_DIRS})
		list(APPEND SHADERTOOL_INCLUDE_DIRS_PARAM "-I")
		list(APPEND SHADERTOOL_INCLUDE_DIRS_PARAM "${IDIR}")
	endforeach()
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
			# TODO We have to add the shader/ dirs of all dependencies to the include path
			add_custom_command(
				OUTPUT ${_shader}.in
				IMPLICIT_DEPENDS C ${_shaders}
				COMMENT "Validate ${_file} and generate ${_shaderfile}"
				COMMAND $<TARGET_FILE:computeshadertool> --shader ${_dir}/${_file} -I ${_dir} ${SHADERTOOL_INCLUDE_DIRS_PARAM} --postfix .in --shadertemplate ${_template} --sourcedir ${GEN_DIR}
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
		COMMAND $<TARGET_FILE:databasetool> --tablefile ${INPUT} --outfile ${GEN_DIR}${OUTPUT}
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
			COMMAND $<TARGET_FILE:flatc> -c -I ${CMAKE_CURRENT_SOURCE_DIR}/../attrib/definitions --scoped-enums -o ${GEN_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/definitions/${DEFINITION}
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
		if (_ADDLIB_UNITY)
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
			message(STATUS "additional lib cflags: ${_ADDLIB_GCCCFLAGS}")
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

macro(engine_add_valgrind TARGET)
	find_program(VALGRIND_EXECUTABLE NAMES valgrind)
	if (VALGRIND_EXECUTABLE)
		add_custom_target(${TARGET}-memcheck)
		add_custom_command(TARGET ${TARGET}-memcheck
			COMMAND
				${VALGRIND_EXECUTABLE} --xml=yes --xml-file=${CMAKE_BINARY_DIR}/memcheck-${TARGET}.xml
				--tool=memcheck --leak-check=full --show-reachable=yes
				--undef-value-errors=yes --track-origins=no --child-silent-after-fork=no
				--trace-children=no --log-file=$<TARGET_FILE:${TARGET}>.memcheck.log
				$<TARGET_FILE:${TARGET}>
			COMMENT "memcheck log for ${TARGET}: $<TARGET_FILE:${TARGET}>.memcheck.log"
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET}
			DEPENDS ${TARGET}
		)
		add_custom_target(${TARGET}-helgrind)
		add_custom_command(TARGET ${TARGET}-helgrind
			COMMAND
				${VALGRIND_EXECUTABLE} --xml=yes --xml-file=${CMAKE_BINARY_DIR}/helgrind-${TARGET}.xml
				--tool=helgrind --child-silent-after-fork=no
				--trace-children=no --log-file=$<TARGET_FILE:${TARGET}>.helgrind.log
				$<TARGET_FILE:${TARGET}>
			COMMENT "helgrind log for ${TARGET}: $<TARGET_FILE:${TARGET}>.helgrind.log"
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET}
			DEPENDS ${TARGET}
		)
	endif()
endmacro()

macro(engine_add_perf TARGET)
	find_program(PERF_EXECUTABLE NAMES perf)
	if (PERF_EXECUTABLE)
		add_custom_target(${TARGET}-perf)
		add_custom_command(TARGET ${TARGET}-perf
			COMMAND
				${PERF_EXECUTABLE} record --call-graph dwarf
				$<TARGET_FILE:${TARGET}>
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET}
			DEPENDS ${TARGET}
		)
	endif()
endmacro()

macro(engina_add_vogl TARGET)
	find_program(VOGL_EXECUTABLE NAMES vogl)
	if (VOGL_EXECUTABLE)
		add_custom_target(${TARGET}-vogl)
		add_custom_command(TARGET ${TARGET}-vogl
			COMMAND
				${VOGL_EXECUTABLE} trace --vogl_tracepath ${CMAKE_BINARY_DIR}
				--vogl_tracefile ${TARGET}.trace.bin
				--vogl_force_debug_context
				$<TARGET_FILE:${TARGET}>
			COMMENT "vogl trace file for ${TARGET}: ${CMAKE_BINARY_DIR}/${TARGET}.trace.bin"
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET}
			DEPENDS ${TARGET}
		)
	endif()
endmacro()

macro(engine_add_debuggger TARGET)
	add_custom_target(${TARGET}-debug)
	if (${DEBUGGER} STREQUAL "gdb")
		add_custom_command(TARGET ${TARGET}-debug
			COMMAND ${GDB_EXECUTABLE} -ex run --args $<TARGET_FILE:${TARGET}>
			COMMENT "Starting debugger session for ${TARGET}"
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET}
			DEPENDS ${TARGET}
		)
	elseif (${DEBUGGER} STREQUAL "lldb")
		add_custom_command(TARGET ${TARGET}-debug
			COMMAND ${LLDB_EXECUTABLE} -b -o run $<TARGET_FILE:${TARGET}>
			COMMENT "Starting debugger session for ${TARGET}"
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET}
			DEPENDS ${TARGET}
		)
	else()
		message(WARN "Unknown DEBUGGER value - set to gdb or lldb")
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

macro(gtest_suite_sources name)
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

macro(gtest_suite_files name)
	if (UNITTESTS)
		set(ARG_LIST ${ARGV})
		list(REMOVE_AT ARG_LIST 0)
		get_property(list GLOBAL PROPERTY ${name}_Files)
		foreach(entry ${ARG_LIST})
			list(APPEND list ${entry})
		endforeach()
		set_property(GLOBAL PROPERTY ${name}_Files ${list})
	endif()
endmacro()

macro(gtest_suite_deps name)
	if (UNITTESTS)
		set(ARG_LIST ${ARGV})
		list(REMOVE_AT ARG_LIST 0)
		get_property(list GLOBAL PROPERTY ${name}_Deps)
		list(APPEND list ${ARG_LIST})
		set_property(GLOBAL PROPERTY ${name}_Deps ${list})
	endif()
endmacro()

macro(gtest_suite_end name)
	if (UNITTESTS)
		project(${name})
		get_property(srcs GLOBAL PROPERTY ${name}_Sources)
		get_property(deps GLOBAL PROPERTY ${name}_Deps)
		generate_unity_sources(SOURCES TARGET ${name} SRCS ${srcs})
		set_target_properties(${name} PROPERTIES OUTPUT_NAME "${CMAKE_PROJECT_NAME}-${name}")
		set_target_properties(${name} PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${name}"
			LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${name}"
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${name}"
		)
		foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
			string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
			set_target_properties(${name} PROPERTIES
				ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/${name}"
				LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/${name}"
				RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/${name}"
			)
		endforeach()

		get_property(models GLOBAL PROPERTY ${name}_Models)
		foreach(entry ${models})
			string(REPLACE ":" ";" inout ${entry})
			list(GET inout 0 in)
			list(GET inout 1 out)
			generate_db_models(${name} ${in} ${out})
		endforeach()

		get_property(files GLOBAL PROPERTY ${name}_Files)
		foreach (datafile ${files})
			string(REPLACE "${name}/" "" target_datafile "${datafile}")
			string(REPLACE "shared/" "" target_datafile "${target_datafile}")
			string(REPLACE "tests/" "" target_datafile "${target_datafile}")
			get_filename_component(datafiledir ${target_datafile} DIRECTORY)
			get_filename_component(filename ${target_datafile} NAME)
			configure_file(${DATA_DIR}/${datafile} ${CMAKE_BINARY_DIR}/${name}/${datafiledir}/${filename} COPYONLY)
		endforeach()
		target_link_libraries(${name} ${deps})
		set_target_properties(${name} PROPERTIES FOLDER ${name})
		add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}>)
		add_custom_target(${name}-run COMMAND $<TARGET_FILE:${name}> DEPENDS ${_EXE_TARGET} WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${name}")
		engine_add_debuggger(${name})
		engine_add_valgrind(${name})
		engine_add_perf(${name})
	endif()
endmacro()

macro(check_lua_files TARGET)
	set(files ${ARGV})
	list(REMOVE_AT files 0)
	find_program(LUAC_EXECUTABLE NAMES ${DEFAULT_LUAC_EXECUTABLE})
	if (LUAC_EXECUTABLE)
		message("${LUA_EXECUTABLE} found")
		foreach(_file ${files})
			get_filename_component(filename ${_file} NAME)
			string(REGEX REPLACE "[/]" "_" targetname ${_file})
			add_custom_target(
				${targetname}
				COMMAND ${LUAC_EXECUTABLE} -o ${CMAKE_CURRENT_BINARY_DIR}/${filename}.out ${_file}
				COMMENT "Validate ${_file}"
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lua/
			)
			add_dependencies(${TARGET} ${targetname})
		endforeach()
	else()
		foreach(_file ${files})
			string(REGEX REPLACE "[/]" "_" targetname ${_file})
			get_filename_component(filename ${_file} NAME)
			add_custom_target(
				${targetname}
				COMMAND $<TARGET_FILE:luac> -o ${CMAKE_CURRENT_BINARY_DIR}/${filename}.out ${_file}
				COMMENT "Validate ${_file}"
				DEPENDS luac
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lua/
			)
			add_dependencies(${TARGET} ${targetname})
		endforeach()
	endif()
endmacro()

macro(check_ui_turbobadger TARGET)
	set(_workingdir "${DATA_DIR}/${TARGET}")
	set(_dir "${_workingdir}/ui/window")
	file(GLOB UI_FILES ${_dir}/*.tb.txt)
	foreach(_file ${UI_FILES})
		get_filename_component(_filename ${_file} NAME)
		add_custom_target(
			${_filename}
			COMMAND $<TARGET_FILE:uitool> ui/window/${_filename}
			COMMENT "Validate ui file: ${_filename}"
			DEPENDS uitool
			WORKING_DIRECTORY ${_workingdir}
		)
		add_dependencies(${TARGET} ${_filename})
	endforeach()
	if (UI_FILES)
		add_dependencies(${TARGET} uitool)
	endif()
endmacro()

macro(engine_install TARGET FILE DESTINATION INSTALL_DATA)
	set(INSTALL_DATA_DIR "${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}-${TARGET}")
	if (INSTALL_DATA)
		install(FILES ${DATA_DIR}/${FILE} DESTINATION ${INSTALL_DATA_DIR}/${DESTINATION} COMPONENT ${TARGET})
	endif()
	get_filename_component(filename ${FILE} NAME)
	configure_file(${DATA_DIR}/${FILE} ${CMAKE_BINARY_DIR}/${TARGET}/${DESTINATION}/${filename} COPYONLY)
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
	set(_MULTI_VALUE_ARGS SRCS LUA_SRCS FILES)

	cmake_parse_arguments(_EXE "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN} )

	# e.g. used in desktop files
	set(COMMANDLINE "${CMAKE_PROJECT_NAME}-${_EXE_TARGET}")
	set(CATEGORIES "Game")
	set(DESCRIPTION "")
	set(NAME ${_EXE_TARGET})
	set(APPICON "${_EXE_TARGET}-icon")
	set(ICON "${APPICON}.png")

	set(${_EXE_TARGET}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "${_EXE_TARGET} source directory")
	if (_EXE_WINDOWED)
		generate_unity_sources(WINDOWED EXECUTABLE TARGET ${_EXE_TARGET} SRCS ${_EXE_SRCS})
	else()
		generate_unity_sources(EXECUTABLE TARGET ${_EXE_TARGET} SRCS ${_EXE_SRCS})
	endif()
	set_target_properties(${_EXE_TARGET} PROPERTIES OUTPUT_NAME "${CMAKE_PROJECT_NAME}-${_EXE_TARGET}")
	set_target_properties(${_EXE_TARGET} PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_EXE_TARGET}"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_EXE_TARGET}"
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_EXE_TARGET}"
	)
	foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
		string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
		set_target_properties(${_EXE_TARGET} PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/${_EXE_TARGET}"
			LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/${_EXE_TARGET}"
			RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/${_EXE_TARGET}"
		)
	endforeach()

	if (_EXE_LUA_SRCS)
		check_lua_files(${_EXE_TARGET} ${_EXE_LUA_SRCS})
	endif()

	set(INSTALL_DATA_DIR "${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}")
	set(INSTALL_ICON_DIR "${CMAKE_INSTALL_DATADIR}/icons")
	set(INSTALL_APPLICATION_DIR "${CMAKE_INSTALL_DATADIR}/applications")

	if (SANITIZER_THREADS AND NOT ${_EXE_TARGET} STREQUAL "databasetool" AND NOT ${_EXE_TARGET} STREQUAL "shadertool" AND NOT ${_EXE_TARGET} STREQUAL "uitool")
		set_target_properties(${_EXE_TARGET} PROPERTIES COMPILE_FLAGS "${SANITIZE_THREAD_FLAG}")
		set_target_properties(${_EXE_TARGET} PROPERTIES LINK_FLAGS "${SANITIZE_THREAD_FLAG}")
	endif()

	if (_EXE_NOINSTALL)
		set(INSTALL_DATA False)
	else()
		set(INSTALL_DATA True)
	endif()

	if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
		if (_EXE_WINDOWED)
			configure_file(${ROOT_DIR}/contrib/installer/linux/desktop.in ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.desktop)
			if (DESKTOP_FILE_VALIDATE_EXECUTABLE)
				add_custom_command(TARGET ${_EXE_TARGET} POST_BUILD
					COMMAND ${DESKTOP_FILE_VALIDATE_EXECUTABLE} ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.desktop
					COMMENT "Validate ${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.desktop"
				)
			endif()
			if (INSTALL_DATA)
				install(FILES ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.desktop DESTINATION ${INSTALL_APPLICATION_DIR})
			endif()
		endif()
		if (EXISTS ${ROOT_DIR}/contrib/installer/linux/${_EXE_TARGET}.service.in)
			# TODO systemd-analyze --user  verify build/Debug/src/server/vengi-server.service
			configure_file(${ROOT_DIR}/contrib/installer/linux/${_EXE_TARGET}.service.in ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.service)
			if (INSTALL_DATA)
				install(FILES ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.service DESTINATION lib/systemd/user)
			endif()
		endif()
	endif()

	set_property(GLOBAL PROPERTY ${_EXE_TARGET}_EXECUTABLE True)
	set_property(GLOBAL PROPERTY ${_EXE_TARGET}_INSTALL ${INSTALL_DATA})
	set_property(GLOBAL PROPERTY ${_EXE_TARGET}_FILES "${_EXE_FILES}")

	foreach (luasrc ${_EXE_LUA_SRCS})
		get_filename_component(luasrcdir ${luasrc} DIRECTORY)
		if (INSTALL_DATA)
			install(FILES lua/${luasrc} DESTINATION ${INSTALL_DATA_DIR}/${luasrcdir} COMPONENT ${_EXE_TARGET})
		endif()
		get_filename_component(filename ${luasrc} NAME)
		get_filename_component(datafiledir ${luasrc} DIRECTORY)
		configure_file(lua/${luasrc} ${CMAKE_BINARY_DIR}/${_EXE_TARGET}/${datafiledir}/${filename} COPYONLY)
	endforeach()

	if (EXISTS ${ROOT_DIR}/contrib/${ICON})
		if (INSTALL_DATA)
			install(FILES ${ROOT_DIR}/contrib/${ICON} DESTINATION ${INSTALL_ICON_DIR} COMPONENT ${_EXE_TARGET})
		endif()
	endif()
	set(KEYBINDINGS "${_EXE_TARGET}-keybindings.cfg")
	if (EXISTS ${ROOT_DIR}/data/${KEYBINDINGS})
		if (INSTALL_DATA)
			install(FILES ${DATA_DIR}/${KEYBINDINGS} DESTINATION ${INSTALL_DATA_DIR}/ COMPONENT ${_EXE_TARGET})
		endif()
		configure_file(${DATA_DIR}/${KEYBINDINGS} ${CMAKE_BINARY_DIR}/${_EXE_TARGET}/${KEYBINDINGS} COPYONLY)
	endif()
	if (INSTALL_DATA)
		install(TARGETS ${_EXE_TARGET} DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${_EXE_TARGET})
	endif()
	add_custom_target(${_EXE_TARGET}-run COMMAND $<TARGET_FILE:${_EXE_TARGET}> DEPENDS ${_EXE_TARGET} WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${_EXE_TARGET}")
	engine_add_debuggger(${_EXE_TARGET})
	engine_add_valgrind(${_EXE_TARGET})
	engine_add_perf(${_EXE_TARGET})
	if (_EXE_WINDOWED)
		engina_add_vogl(${_EXE_TARGET})
	endif()
endmacro()

macro(engine_add_module)
	set(_OPTIONS_ARGS)
	set(_ONE_VALUE_ARGS TARGET)
	set(_MULTI_VALUE_ARGS SRCS FILES DEPENDENCIES)

	cmake_parse_arguments(_LIB "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN})

	set(${_LIB_TARGET}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "${_LIB_TARGET} module source directory")
	generate_unity_sources(LIBRARY TARGET ${_LIB_TARGET} SRCS ${_LIB_SRCS})

	set_target_properties(${_LIB_TARGET} PROPERTIES FOLDER ${_LIB_TARGET})
	if (_LIB_DEPENDENCIES)
		target_link_libraries(${_LIB_TARGET} ${_LIB_DEPENDENCIES})
		foreach (dep ${_LIB_DEPENDENCIES})
			get_property(DEP_FILES GLOBAL PROPERTY ${dep}_FILES)
			list(APPEND _LIB_FILES ${DEP_FILES})
		endforeach()
	endif()
	set_property(GLOBAL PROPERTY ${_LIB_TARGET}_FILES ${_LIB_FILES})
	set_property(GLOBAL PROPERTY ${_LIB_TARGET}_DEPENDENCIES ${_LIB_DEPENDENCIES})
endmacro()

macro(engine_target_link_libraries)
	set(_OPTIONS_ARGS)
	set(_ONE_VALUE_ARGS TARGET)
	set(_MULTI_VALUE_ARGS DEPENDENCIES)

	cmake_parse_arguments(_LIBS "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN})

	set_property(GLOBAL PROPERTY ${_LIBS_TARGET}_DEPENDENCIES ${_LIBS_DEPENDENCIES})
	target_link_libraries(${_LIBS_TARGET} ${_LIBS_DEPENDENCIES})

	get_property(EXECUTABLE GLOBAL PROPERTY ${_LIBS_TARGET}_EXECUTABLE)
	if (EXECUTABLE)
		get_property(INSTALL_DATA GLOBAL PROPERTY ${_LIBS_TARGET}_INSTALL)
		get_property(INSTALL_FILES GLOBAL PROPERTY ${_LIBS_TARGET}_FILES)
		foreach (dep ${_LIBS_DEPENDENCIES})
			get_property(FILES GLOBAL PROPERTY ${dep}_FILES)
			list(APPEND INSTALL_FILES ${FILES})
		endforeach()

		if (INSTALL_FILES)
			list(REMOVE_DUPLICATES INSTALL_FILES)
			foreach (datafile ${INSTALL_FILES})
				string(REPLACE "${_LIBS_TARGET}/" "" target_datafile "${datafile}")
				string(REPLACE "shared/" "" target_datafile "${target_datafile}")
				get_filename_component(datafiledir ${target_datafile} DIRECTORY)
				engine_install(${_LIBS_TARGET} "${datafile}" "${datafiledir}" ${INSTALL_DATA})
			endforeach()
		endif()
	endif()
endmacro()
