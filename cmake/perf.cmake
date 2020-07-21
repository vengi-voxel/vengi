macro(engine_add_perf TARGET)
	find_program(PERF_EXECUTABLE NAMES perf)
	if (PERF_EXECUTABLE)
		add_custom_target(${TARGET}-perf)
		add_custom_command(TARGET ${TARGET}-perf
			COMMAND
				${PERF_EXECUTABLE} record --call-graph dwarf
				$<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET}
			DEPENDS ${TARGET}
		)
	endif()
endmacro()
