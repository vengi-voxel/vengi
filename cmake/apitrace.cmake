function(engine_add_apitrace TARGET)
	find_program(APITRACE_EXECUTABLE NAMES apitrace)
	if (APITRACE_EXECUTABLE)
		add_custom_target(${TARGET}-apitrace)
		add_custom_command(TARGET ${TARGET}-apitrace
			COMMAND
				${APITRACE_EXECUTABLE} trace --api gl
				$<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET}
			DEPENDS ${TARGET}
		)
	endif()
endfunction()
