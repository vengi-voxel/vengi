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
