function(engine_add_heaptrack TARGET)
	find_program(HEAPTRACK_EXECUTABLE NAMES heaptrack)
	if (HEAPTRACK_EXECUTABLE)
		add_custom_target(${TARGET}-heaptrack)
		add_custom_command(TARGET ${TARGET}-heaptrack
			COMMAND
				${HEAPTRACK_EXECUTABLE}
				$<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET}
			DEPENDS ${TARGET}
		)
	endif()
endfunction()
