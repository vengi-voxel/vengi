function(engine_add_valgrind TARGET)
	find_program(VALGRIND_EXECUTABLE NAMES valgrind)
	if (VALGRIND_EXECUTABLE)
		add_custom_target(${TARGET}-memcheckxml
			COMMAND
				${VALGRIND_EXECUTABLE} --xml=yes --xml-file=${CMAKE_CURRENT_BINARY_DIR}/memcheck-${TARGET}.xml
				--tool=memcheck --leak-check=full --show-reachable=yes
				--undef-value-errors=yes --track-origins=yes --child-silent-after-fork=no
				--trace-children=yes --track-fds=yes
				$<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			DEPENDS ${TARGET}
			WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET}>
			COMMENT "Executing valgrind memcheck and log into ${CMAKE_CURRENT_BINARY_DIR}"
		)
		add_custom_target(${TARGET}-memcheck
			COMMAND
				${VALGRIND_EXECUTABLE} --tool=memcheck --leak-check=full --show-reachable=yes
				--undef-value-errors=yes --track-origins=yes --child-silent-after-fork=no
				--trace-children=yes --track-fds=yes --log-file=${CMAKE_CURRENT_BINARY_DIR}/memcheck-${TARGET}.log
				$<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			DEPENDS ${TARGET}
			WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET}>
			COMMENT "Executing valgrind memcheck and log into ${CMAKE_CURRENT_BINARY_DIR}"
		)
		add_custom_target(${TARGET}-helgrind
			COMMAND
				${VALGRIND_EXECUTABLE} --xml=yes --xml-file=${CMAKE_BINARY_DIR}/helgrind-${TARGET}.xml
				--tool=helgrind --child-silent-after-fork=no
				--trace-children=no --log-file=$<TARGET_FILE:${TARGET}>.helgrind.log
				$<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			DEPENDS ${TARGET}
			WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET}>
			COMMENT "Executing valgrind helgrind and log into ${CMAKE_CURRENT_BINARY_DIR}"
		)
	endif()
endfunction()
