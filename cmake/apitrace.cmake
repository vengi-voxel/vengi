function(engine_add_apitrace TARGET)
	find_program(APITRACE_EXECUTABLE NAMES apitrace)
	if (APITRACE_EXECUTABLE)
		add_custom_target(${TARGET}-apitrace
			COMMAND ${APITRACE_EXECUTABLE} trace --api gl $<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			DEPENDS ${TARGET}
			WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET}>
		)
	endif()
endfunction()
