macro(check_lua_files TARGET)
	set(files ${ARGV})
	list(REMOVE_AT files 0)
	find_program(LUAC_EXECUTABLE NAMES ${DEFAULT_LUAC_EXECUTABLE})
	set(ADDITIONAL_DEPENDENCIES)
	if (LUAC_EXECUTABLE)
		message(STATUS "${LUAC_EXECUTABLE} found")
	else()
		set(LUAC_EXECUTABLE luac)
		list(APPEND ADDITIONAL_DEPENDENCIES luac)
	endif()
	foreach(_file ${files})
		string(REGEX REPLACE "[/]" "_" targetname ${_file})
		get_filename_component(filename ${_file} NAME)
		set(_outfile ${targetname}.out)
		add_custom_command(
			OUTPUT ${_outfile}
			COMMAND ${LUAC_EXECUTABLE} -o ${_outfile} ${CMAKE_CURRENT_SOURCE_DIR}/lua/${_file}
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lua/${_file} ${ADDITIONAL_DEPENDENCIES}
			COMMENT "Validate ${_file}"
		)
		add_custom_target(${TARGET}-${_outfile} DEPENDS ${_outfile})
		add_dependencies(${TARGET} ${TARGET}-${_outfile})
	endforeach()
endmacro()
