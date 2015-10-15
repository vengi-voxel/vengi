SET(DEFAULT_LUA_EXECUTABLE lua)

macro(copy_data_files TARGET)
	add_custom_target(copy-data-${TARGET} ALL
		COMMAND cmake -E copy_directory "${FIPS_PROJECT_DIR}/data/${TARGET}/" ${FIPS_DEPLOY_DIR}/${CMAKE_PROJECT_NAME}/${FIPS_CONFIG}
		COMMENT "Copy ${TARGET} data files...")
endmacro()

macro(check_lua_files TARGET files)
	find_program(LUA_EXECUTABLE NAMES ${DEFAULT_LUA_EXECUTABLE})
	if (LUA_EXECUTABLE)
		message("${LUA_EXECUTABLE} found")
		foreach(_file ${files})
			add_custom_target(
				${_file}
				COMMAND ${LUA_EXECUTABLE} ${_file}
				WORKING_DIRECTORY ${FIPS_PROJECT_DIR}/data/${TARGET}
			)
			add_dependencies(${TARGET} ${_file})
		endforeach()
	else()
		message(WARNING "No ${DEFAULT_LUA_EXECUTABLE} found")
	endif()
endmacro()
