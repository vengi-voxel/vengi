function(engine_add_valgrind TARGET)
	find_program(VALGRIND_EXECUTABLE NAMES valgrind)
	if (VALGRIND_EXECUTABLE)
		add_custom_target(${TARGET}-memcheck
			COMMAND
				${VALGRIND_EXECUTABLE} --tool=memcheck --leak-check=full --show-reachable=yes
				--undef-value-errors=yes --track-origins=yes --child-silent-after-fork=no
				--trace-children=yes --track-fds=yes --log-file=${CMAKE_CURRENT_BINARY_DIR}/memcheck-${TARGET}.log
				--xml-file=${CMAKE_CURRENT_BINARY_DIR}/memcheck-${TARGET}.xml
				$<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			DEPENDS ${TARGET}
			WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET}>
			COMMENT "Executing valgrind memcheck and log into ${CMAKE_CURRENT_BINARY_DIR}"
		)
		add_custom_target(${TARGET}-helgrind
			COMMAND
				${VALGRIND_EXECUTABLE} --xml=yes
				--tool=helgrind --child-silent-after-fork=no
				--trace-children=no --log-file=${CMAKE_CURRENT_BINARY_DIR}/helgrind-${TARGET}.log
				--xml-file=${CMAKE_BINARY_DIR}/helgrind-${TARGET}.xml
				$<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			DEPENDS ${TARGET}
			WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET}>
			COMMENT "Executing valgrind helgrind and log into ${CMAKE_CURRENT_BINARY_DIR}"
		)
		add_custom_target(${TARGET}-cachegrind
			COMMAND
				${VALGRIND_EXECUTABLE}
				--tool=cachegrind --branch-sim=yes --log-file=${CMAKE_CURRENT_BINARY_DIR}/cachegrind-${TARGET}.log
				$<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			DEPENDS ${TARGET}
			WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET}>
			COMMENT "Executing valgrind cachegrind and log into ${CMAKE_CURRENT_BINARY_DIR}"
		)
		add_custom_target(${TARGET}-callgrind
			COMMAND
				${VALGRIND_EXECUTABLE}
				--tool=callgrind --dump-instr=yes --log-file=${CMAKE_CURRENT_BINARY_DIR}/callgrind-${TARGET}.log
				$<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			DEPENDS ${TARGET}
			WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET}>
			COMMENT "Executing valgrind callgrind and log into ${CMAKE_CURRENT_BINARY_DIR}"
		)
		add_custom_target(${TARGET}-drd
			COMMAND
				${VALGRIND_EXECUTABLE} --xml=yes
				--tool=drd --child-silent-after-fork=no
				--trace-children=no --log-file=${CMAKE_CURRENT_BINARY_DIR}/drd-${TARGET}.log
				--xml-file=${CMAKE_BINARY_DIR}/drd-${TARGET}.xml
				$<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			DEPENDS ${TARGET}
			WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET}>
			COMMENT "Executing valgrind drd and log into ${CMAKE_CURRENT_BINARY_DIR}"
		)
		add_custom_target(${TARGET}-massif
			COMMAND
				${VALGRIND_EXECUTABLE} --xml=yes
				--tool=massif --child-silent-after-fork=no
				--trace-children=no --log-file=${CMAKE_CURRENT_BINARY_DIR}/massif-${TARGET}.log
				--xml-file=${CMAKE_BINARY_DIR}/massif-${TARGET}.xml
				$<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			DEPENDS ${TARGET}
			WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET}>
			COMMENT "Executing valgrind massif and log into ${CMAKE_CURRENT_BINARY_DIR}"
		)
		add_custom_target(${TARGET}-dhat
			COMMAND
				${VALGRIND_EXECUTABLE}
				--tool=dhat --log-file=${CMAKE_CURRENT_BINARY_DIR}/dhat-${TARGET}.log
				$<TARGET_FILE:${TARGET}>
			USES_TERMINAL
			DEPENDS ${TARGET}
			WORKING_DIRECTORY $<TARGET_FILE_DIR:${TARGET}>
			COMMENT "Executing valgrind dhat and log into ${CMAKE_CURRENT_BINARY_DIR}"
		)
	endif()
endfunction()
