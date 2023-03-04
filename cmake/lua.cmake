function(check_lua_files TARGET)
	if (NOT CMAKE_CROSSCOMPILING)
		set(files ${ARGN})
		foreach(_file ${files})
			string(REGEX REPLACE "[/]" "_" targetname ${_file})
			get_filename_component(filename ${_file} NAME)
			set(_outfile ${targetname}.out)
			add_custom_command(
				OUTPUT ${_outfile}
				COMMAND $<TARGET_FILE:luac> -o ${_outfile} ${CMAKE_CURRENT_SOURCE_DIR}/lua/${_file}
				DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lua/${_file}
				COMMENT "Validate ${_file}"
			)
			add_custom_target(${TARGET}-${_outfile} DEPENDS ${_outfile})
			add_dependencies(${TARGET} ${TARGET}-${_outfile})
		endforeach()
	endif()
endfunction()
