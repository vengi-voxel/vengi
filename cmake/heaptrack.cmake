function(engine_add_heaptrack TARGET)
	find_program(HEAPTRACK_EXECUTABLE NAMES heaptrack)
	if (HEAPTRACK_EXECUTABLE)
		add_custom_target(${TARGET}-heaptrack
			COMMAND ${HEAPTRACK_EXECUTABLE} $<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			DEPENDS ${TARGET}
			WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET}>
		)
	endif()
endfunction()
