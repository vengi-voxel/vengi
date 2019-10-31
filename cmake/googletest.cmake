
#-------------------------------------------------------------------------------
#   Macros for generating google unit tests.
#-------------------------------------------------------------------------------

set(GOOGLETESTDIR ${CMAKE_CURRENT_LIST_DIR})

find_package(GTest)

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
		set_property(GLOBAL PROPERTY ${name}_EXECUTABLE True)
		set_property(GLOBAL PROPERTY ${name}_INSTALL False)

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

		target_compile_options(${name} PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wno-undef>)
		get_property(models GLOBAL PROPERTY ${name}_Models)
		foreach(entry ${models})
			string(REPLACE ":" ";" inout ${entry})
			list(GET inout 0 in)
			list(GET inout 1 out)
			generate_db_models(${name} ${in} ${out})
		endforeach()

		get_property(files GLOBAL PROPERTY ${name}_Files)
		foreach (datafile ${files})
			string(REGEX REPLACE "^[^/]+" "" target_datafile "${datafile}")
			string(LENGTH ${target_datafile} target_datafile_length)
			string(SUBSTRING ${target_datafile} 1 ${target_datafile_length} target_datafile)
			get_filename_component(datafiledir ${target_datafile} DIRECTORY)
			get_filename_component(filename ${target_datafile} NAME)
			configure_file(${DATA_DIR}/${datafile} ${CMAKE_BINARY_DIR}/${name}/${datafiledir}/${filename} COPYONLY)
		endforeach()
		engine_target_link_libraries(TARGET ${name} DEPENDENCIES ${deps})
		set_target_properties(${name} PROPERTIES FOLDER ${name})
		add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}> --gtest_output=xml:${CMAKE_BINARY_DIR}/${name}.xml WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${name}")
		add_custom_target(${name}-run COMMAND $<TARGET_FILE:${name}> --gtest_output=xml:${CMAKE_BINARY_DIR}/${name}.xml DEPENDS ${name} WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${name}")
		engine_add_debuggger(${name})
		engine_add_valgrind(${name})
		engine_add_perf(${name})
	endif()
endmacro()
