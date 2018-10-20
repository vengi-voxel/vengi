macro(check_lua_files TARGET)
	set(files ${ARGV})
	list(REMOVE_AT files 0)
	find_program(LUAC_EXECUTABLE NAMES ${DEFAULT_LUAC_EXECUTABLE})
	if (LUAC_EXECUTABLE)
		message(STATUS "${LUAC_EXECUTABLE} found")
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
