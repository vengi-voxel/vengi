macro(engina_add_vogl TARGET)
	find_program(VOGL_EXECUTABLE NAMES vogl)
	if (VOGL_EXECUTABLE)
		add_custom_target(${TARGET}-vogl)
		add_custom_command(TARGET ${TARGET}-vogl
			COMMAND
				${VOGL_EXECUTABLE} trace --vogl_tracepath ${CMAKE_BINARY_DIR}
				--vogl_tracefile ${TARGET}.trace.bin
				--vogl_force_debug_context
				$<TARGET_FILE:${TARGET}>
			COMMENT "vogl trace file for ${TARGET}: ${CMAKE_BINARY_DIR}/${TARGET}.trace.bin"
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET}
			DEPENDS ${TARGET}
		)
	endif()
endmacro()
