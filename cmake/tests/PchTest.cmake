# Standalone build regression: C/C++, PIC/PIE, shared donors and disabled PCH.
foreach(_mode enabled disabled no-tests no-pic)
	set(_options)
	if (_mode STREQUAL "disabled")
		list(APPEND _options -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON)
	elseif (_mode STREQUAL "no-tests")
		list(APPEND _options -DUNITTESTS=OFF)
	elseif (_mode STREQUAL "no-pic")
		list(APPEND _options -DCMAKE_POSITION_INDEPENDENT_CODE=OFF)
	endif()
	set(_build "${PCH_TEST_BINARY_DIR}/${_mode}")
	execute_process(COMMAND "${CMAKE_COMMAND}" -S "${CMAKE_CURRENT_LIST_DIR}/pch" -B "${_build}"
		-G "${PCH_TEST_GENERATOR}" "-DCMAKE_C_COMPILER=${PCH_TEST_C_COMPILER}"
		"-DCMAKE_CXX_COMPILER=${PCH_TEST_CXX_COMPILER}" ${_options}
		RESULT_VARIABLE _result OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr)
	if (_result)
		message(FATAL_ERROR "PCH ${_mode} configure failed:\n${_stdout}\n${_stderr}")
	endif()
	execute_process(COMMAND "${CMAKE_COMMAND}" --build "${_build}" --config Debug --parallel 2
		RESULT_VARIABLE _result OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr)
	if (_result)
		message(FATAL_ERROR "PCH ${_mode} build failed:\n${_stdout}\n${_stderr}")
	endif()
	# Keep successful regression builds from accumulating additional PCH files.
	file(REMOVE_RECURSE "${_build}")
	message(STATUS "PCH ${_mode} passed")
endforeach()
