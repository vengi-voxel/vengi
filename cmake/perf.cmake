function(engine_add_perf TARGET)
	find_program(PERF_EXECUTABLE NAMES perf)
	if (PERF_EXECUTABLE)
		add_custom_target(${TARGET}-perf
			COMMAND
				${PERF_EXECUTABLE} record --call-graph dwarf
				$<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			DEPENDS ${TARGET}
			WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET}>
		)
	endif()
endfunction()
