# LineCount.cmake - drop-in include-bloat analyser for any CMake project.
#
# Preprocess-only (no link). Uses each target's INCLUDE_DIRECTORIES /
# COMPILE_DEFINITIONS. Creates:
#   linecount                 headers alone (path:lines, sorted)
#   linecount-<target>        headers of one target
#   linecount-cpp             .cpp TU totals (path:lines, sorted)
#   linecount-cpp-<target>    .cpp totals for one target
#   linecount-why-<target>-<name>   include contribution breakdown for one .cpp
#
# -----------------------------------------------------------------------------
# Usage (copy this single file):
#
#   set(LINECOUNT_DIR_REGEX "${CMAKE_SOURCE_DIR}/src")  # optional
#   include(cmake/LineCount.cmake)
#   add_library(mylib ...)
#   linecount_finalize()
#
#   cmake --build build --target linecount
#   cmake --build build --target linecount-cpp
#   cmake --build build --target linecount-why-mylib-Foo_cpp
#
# Why is Foo.cpp huge?
#   1) linecount-cpp          -> see Foo.cpp:N
#   2) linecount-why-...-Foo  -> see which headers contribute N lines
# -----------------------------------------------------------------------------
# Also used as cmake -P with LINECOUNT_MODE=one|report|why.

cmake_minimum_required(VERSION 3.15)

if (CMAKE_SCRIPT_MODE_FILE)

	function(_linecount_load_flags flags_file is_msvc out_var)
		set(_flags)
		file(STRINGS "${flags_file}" _flag_lines)
		foreach (_line ${_flag_lines})
			if (_line MATCHES "^INCLUDE:(.+)$")
				if (is_msvc)
					list(APPEND _flags "/I${CMAKE_MATCH_1}")
				else()
					list(APPEND _flags "-I${CMAKE_MATCH_1}")
				endif()
			elseif (_line MATCHES "^DEFINE:(.+)$")
				if (is_msvc)
					list(APPEND _flags "/D${CMAKE_MATCH_1}")
				else()
					list(APPEND _flags "-D${CMAKE_MATCH_1}")
				endif()
			endif()
		endforeach()
		set(${out_var} "${_flags}" PARENT_SCOPE)
	endfunction()

	function(_linecount_pretty_path abs_path out_var)
		set(_root "${LINECOUNT_SOURCE_ROOT}")
		if (NOT _root)
			get_filename_component(_root "${abs_path}" DIRECTORY)
		endif()
		file(RELATIVE_PATH _rel "${_root}" "${abs_path}")
		if (LINECOUNT_STRIP_PREFIX)
			string(LENGTH "${LINECOUNT_STRIP_PREFIX}" _plen)
			if (_plen GREATER 0)
				string(SUBSTRING "${_rel}" 0 ${_plen} _prefix)
				if (_prefix STREQUAL "${LINECOUNT_STRIP_PREFIX}")
					string(SUBSTRING "${_rel}" ${_plen} -1 _rel)
				endif()
			endif()
		endif()
		# Keep absolute system paths readable as-is when outside the tree.
		if (_rel MATCHES "^\\.\\./")
			set(_rel "${abs_path}")
		endif()
		set(${out_var} "${_rel}" PARENT_SCOPE)
	endfunction()

	function(_linecount_pad_sort entries_var)
		set(_entries ${${entries_var}})
		list(SORT _entries)
		list(REVERSE _entries)
		set(${entries_var} "${_entries}" PARENT_SCOPE)
	endfunction()

	function(_linecount_pad count out_var)
		string(LENGTH "${count}" _len)
		math(EXPR _pad "10 - ${_len}")
		if (_pad LESS 0)
			set(_pad 0)
		endif()
		set(_padded "${count}")
		if (_pad GREATER 0)
			string(REPEAT "0" ${_pad} _zeros)
			set(_padded "${_zeros}${count}")
		endif()
		set(${out_var} "${_padded}" PARENT_SCOPE)
	endfunction()

	if (LINECOUNT_MODE STREQUAL "one")
		if (NOT LINECOUNT_COMPILER OR NOT LINECOUNT_STUB OR NOT LINECOUNT_COUNT_FILE OR NOT LINECOUNT_FLAGS_FILE)
			message(FATAL_ERROR "LineCount one: missing required variables")
		endif()
		if (NOT EXISTS "${LINECOUNT_FLAGS_FILE}")
			message(FATAL_ERROR "LineCount one: flags file not found: ${LINECOUNT_FLAGS_FILE}")
		endif()
		if (NOT LINECOUNT_DISPLAY)
			get_filename_component(LINECOUNT_DISPLAY "${LINECOUNT_STUB}" NAME)
		endif()
		if (NOT LINECOUNT_CXX_STD)
			set(LINECOUNT_CXX_STD 17)
		endif()

		_linecount_load_flags("${LINECOUNT_FLAGS_FILE}" "${LINECOUNT_IS_MSVC}" _flags)
		get_filename_component(_outdir "${LINECOUNT_COUNT_FILE}" DIRECTORY)
		file(MAKE_DIRECTORY "${_outdir}")
		set(_preprocessed "${LINECOUNT_COUNT_FILE}.i")

		if (LINECOUNT_IS_MSVC)
			execute_process(
				COMMAND "${LINECOUNT_COMPILER}" /nologo /EP /TP "/std:c++${LINECOUNT_CXX_STD}" ${_flags} "${LINECOUNT_STUB}"
				OUTPUT_FILE "${_preprocessed}"
				ERROR_VARIABLE _err
				RESULT_VARIABLE _rc
			)
		else()
			execute_process(
				COMMAND "${LINECOUNT_COMPILER}" "-std=c++${LINECOUNT_CXX_STD}" -P -E -x c++ ${_flags} "${LINECOUNT_STUB}"
				OUTPUT_FILE "${_preprocessed}"
				ERROR_VARIABLE _err
				RESULT_VARIABLE _rc
			)
		endif()

		if (NOT _rc EQUAL 0)
			file(WRITE "${LINECOUNT_COUNT_FILE}" "${LINECOUNT_DISPLAY}:FAILED\n")
			message(WARNING "linecount failed for ${LINECOUNT_DISPLAY}: ${_err}")
			return()
		endif()

		file(STRINGS "${_preprocessed}" _lines)
		list(LENGTH _lines _count)
		file(WRITE "${LINECOUNT_COUNT_FILE}" "${LINECOUNT_DISPLAY}:${_count}\n")
		file(REMOVE "${_preprocessed}")
		return()
	endif()

	if (LINECOUNT_MODE STREQUAL "report")
		if (NOT LINECOUNT_REPORT)
			message(FATAL_ERROR "LineCount report: LINECOUNT_REPORT required")
		endif()
		if (LINECOUNT_FILTER_LIST)
			file(STRINGS "${LINECOUNT_FILTER_LIST}" _files)
		elseif (LINECOUNT_COUNTS_DIR)
			file(GLOB _files "${LINECOUNT_COUNTS_DIR}/*.count")
		else()
			message(FATAL_ERROR "LineCount report: set LINECOUNT_COUNTS_DIR or LINECOUNT_FILTER_LIST")
		endif()

		set(_entries)
		foreach (_file ${_files})
			if (NOT EXISTS "${_file}")
				continue()
			endif()
			file(STRINGS "${_file}" _line LIMIT_COUNT 1)
			if (NOT _line MATCHES "^(.+):([0-9]+)$")
				continue()
			endif()
			_linecount_pad("${CMAKE_MATCH_2}" _padded)
			list(APPEND _entries "${_padded}|${CMAKE_MATCH_1}:${CMAKE_MATCH_2}")
		endforeach()

		_linecount_pad_sort(_entries)
		set(_body)
		foreach (_entry ${_entries})
			string(REGEX REPLACE "^[^|]+\\|" "" _line "${_entry}")
			string(APPEND _body "${_line}\n")
			message(STATUS "${_line}")
		endforeach()

		get_filename_component(_reportdir "${LINECOUNT_REPORT}" DIRECTORY)
		file(MAKE_DIRECTORY "${_reportdir}")
		file(WRITE "${LINECOUNT_REPORT}" "${_body}")
		list(LENGTH _entries _n)
		message(STATUS "Wrote ${_n} line counts to ${LINECOUNT_REPORT}")
		return()
	endif()

	# Attribute preprocessed lines of a .cpp to each contributing source file
	# using compiler #line / # N "file" markers (not stripped with -P /EP).
	if (LINECOUNT_MODE STREQUAL "why")
		if (NOT LINECOUNT_COMPILER OR NOT LINECOUNT_SOURCE OR NOT LINECOUNT_REPORT OR NOT LINECOUNT_FLAGS_FILE)
			message(FATAL_ERROR "LineCount why: missing required variables")
		endif()
		if (NOT EXISTS "${LINECOUNT_SOURCE}")
			message(FATAL_ERROR "LineCount why: source not found: ${LINECOUNT_SOURCE}")
		endif()
		if (NOT LINECOUNT_CXX_STD)
			set(LINECOUNT_CXX_STD 17)
		endif()
		if (NOT LINECOUNT_TOP)
			set(LINECOUNT_TOP 40)
		endif()

		_linecount_load_flags("${LINECOUNT_FLAGS_FILE}" "${LINECOUNT_IS_MSVC}" _flags)
		get_filename_component(_outdir "${LINECOUNT_REPORT}" DIRECTORY)
		file(MAKE_DIRECTORY "${_outdir}")
		set(_preprocessed "${LINECOUNT_REPORT}.i")

		# Keep line markers: GCC/Clang -E (no -P), MSVC /E (no /EP).
		if (LINECOUNT_IS_MSVC)
			execute_process(
				COMMAND "${LINECOUNT_COMPILER}" /nologo /E /TP "/std:c++${LINECOUNT_CXX_STD}" ${_flags} "${LINECOUNT_SOURCE}"
				OUTPUT_FILE "${_preprocessed}"
				ERROR_VARIABLE _err
				RESULT_VARIABLE _rc
			)
		else()
			execute_process(
				COMMAND "${LINECOUNT_COMPILER}" "-std=c++${LINECOUNT_CXX_STD}" -E -x c++ ${_flags} "${LINECOUNT_SOURCE}"
				OUTPUT_FILE "${_preprocessed}"
				ERROR_VARIABLE _err
				RESULT_VARIABLE _rc
			)
		endif()

		if (NOT _rc EQUAL 0)
			file(WRITE "${LINECOUNT_REPORT}" "FAILED\n${_err}\n")
			message(WARNING "linecount-why failed for ${LINECOUNT_SOURCE}: ${_err}")
			return()
		endif()

		file(STRINGS "${_preprocessed}" _lines)
		set(_current "")
		set(_total 0)
		# counts keyed via parallel lists (pure cmake, no maps)
		set(_files)
		set(_counts)

		foreach (_line ${_lines})
			set(_is_marker FALSE)
			set(_marker_file "")
			# GCC/Clang: # linenum "file" [flags]
			# MSVC:      #line linenum "file"
			if (_line MATCHES "^#line[ \t]+[0-9]+[ \t]+\"([^\"]+)\"")
				set(_is_marker TRUE)
				set(_marker_file "${CMAKE_MATCH_1}")
			elseif (_line MATCHES "^#[ \t]*[0-9]+[ \t]+\"([^\"]+)\"")
				set(_is_marker TRUE)
				set(_marker_file "${CMAKE_MATCH_1}")
			endif()

			if (_is_marker)
				set(_current "${_marker_file}")
				# Ignore builtin/command-line pseudo files.
				if (_current MATCHES "^<.*>$")
					set(_current "")
				endif()
				continue()
			endif()

			if (_current STREQUAL "")
				continue()
			endif()

			math(EXPR _total "${_total} + 1")
			list(FIND _files "${_current}" _idx)
			if (_idx EQUAL -1)
				list(APPEND _files "${_current}")
				list(APPEND _counts 1)
			else()
				list(GET _counts ${_idx} _c)
				math(EXPR _c "${_c} + 1")
				list(REMOVE_AT _counts ${_idx})
				list(INSERT _counts ${_idx} ${_c})
			endif()
		endforeach()

		file(REMOVE "${_preprocessed}")

		_linecount_pretty_path("${LINECOUNT_SOURCE}" _src_display)
		set(_entries)
		set(_i 0)
		list(LENGTH _files _nfiles)
		while (_i LESS _nfiles)
			list(GET _files ${_i} _f)
			list(GET _counts ${_i} _c)
			_linecount_pretty_path("${_f}" _disp)
			_linecount_pad("${_c}" _padded)
			list(APPEND _entries "${_padded}|${_disp}:${_c}")
			math(EXPR _i "${_i} + 1")
		endwhile()

		_linecount_pad_sort(_entries)

		set(_body)
		string(APPEND _body "TU ${_src_display}:${_total}\n")
		string(APPEND _body "---- include contributions (top ${LINECOUNT_TOP}) ----\n")
		message(STATUS "TU ${_src_display}:${_total}")
		message(STATUS "---- include contributions (top ${LINECOUNT_TOP}) ----")

		set(_shown 0)
		foreach (_entry ${_entries})
			string(REGEX REPLACE "^[^|]+\\|" "" _line "${_entry}")
			string(APPEND _body "${_line}\n")
			message(STATUS "${_line}")
			math(EXPR _shown "${_shown} + 1")
			if (_shown GREATER_EQUAL LINECOUNT_TOP)
				break()
			endif()
		endforeach()

		file(WRITE "${LINECOUNT_REPORT}" "${_body}")
		message(STATUS "Wrote breakdown to ${LINECOUNT_REPORT}")
		return()
	endif()

	if (LINECOUNT_MODE STREQUAL "show")
		if (NOT LINECOUNT_REPORT OR NOT EXISTS "${LINECOUNT_REPORT}")
			message(FATAL_ERROR "LineCount show: report not found: ${LINECOUNT_REPORT}")
		endif()
		file(STRINGS "${LINECOUNT_REPORT}" _lines)
		foreach (_line ${_lines})
			message(STATUS "${_line}")
		endforeach()
		return()
	endif()

	message(FATAL_ERROR "LineCount: unknown LINECOUNT_MODE='${LINECOUNT_MODE}' (expected one|report|why|show)")
endif()

# ---- configure-time API ----------------------------------------------------

set(LINECOUNT_MODULE_FILE "${CMAKE_CURRENT_LIST_FILE}" CACHE INTERNAL "Path to LineCount.cmake")

if (NOT DEFINED LINECOUNT_AUTO_FINALIZE)
	set(LINECOUNT_AUTO_FINALIZE ON)
endif()
if (NOT DEFINED LINECOUNT_AUTO_REGISTER)
	set(LINECOUNT_AUTO_REGISTER ON)
endif()

function(linecount_register TARGET)
	if (NOT TARGET ${TARGET})
		return()
	endif()
	get_target_property(_type ${TARGET} TYPE)
	if (_type STREQUAL "INTERFACE_LIBRARY")
		return()
	endif()
	if (_type STREQUAL "UNKNOWN_LIBRARY")
		return()
	endif()
	get_property(_targets GLOBAL PROPERTY LINECOUNT_TARGETS)
	list(FIND _targets ${TARGET} _idx)
	if (NOT _idx EQUAL -1)
		return()
	endif()
	list(APPEND _targets ${TARGET})
	set_property(GLOBAL PROPERTY LINECOUNT_TARGETS "${_targets}")
endfunction()

function(_linecount_maybe_register)
	if (NOT LINECOUNT_AUTO_REGISTER)
		return()
	endif()
	if (NOT ARGN)
		return()
	endif()
	list(GET ARGN 0 _tgt)
	if ("ALIAS" IN_LIST ARGN OR "IMPORTED" IN_LIST ARGN)
		return()
	endif()
	if (LINECOUNT_DIR_REGEX)
		if (NOT CMAKE_CURRENT_SOURCE_DIR MATCHES "${LINECOUNT_DIR_REGEX}")
			return()
		endif()
	endif()
	linecount_register(${_tgt})
endfunction()

get_property(_linecount_wrapped GLOBAL PROPERTY LINECOUNT_COMMANDS_WRAPPED)
if (LINECOUNT_AUTO_REGISTER AND NOT _linecount_wrapped)
	set_property(GLOBAL PROPERTY LINECOUNT_COMMANDS_WRAPPED TRUE)

	function(add_library)
		_add_library(${ARGN})
		_linecount_maybe_register(${ARGN})
	endfunction()

	function(add_executable)
		_add_executable(${ARGN})
		_linecount_maybe_register(${ARGN})
	endfunction()
endif()

function(_linecount_display_path abs_path OUT_VAR)
	if (LINECOUNT_SOURCE_ROOT)
		set(_root "${LINECOUNT_SOURCE_ROOT}")
	else()
		set(_root "${CMAKE_SOURCE_DIR}")
	endif()
	file(RELATIVE_PATH _rel "${_root}" "${abs_path}")
	if (LINECOUNT_STRIP_PREFIX)
		string(LENGTH "${LINECOUNT_STRIP_PREFIX}" _plen)
		string(SUBSTRING "${_rel}" 0 ${_plen} _prefix)
		if (_prefix STREQUAL "${LINECOUNT_STRIP_PREFIX}")
			string(SUBSTRING "${_rel}" ${_plen} -1 _rel)
		endif()
	endif()
	set(${OUT_VAR} "${_rel}" PARENT_SCOPE)
endfunction()

function(_linecount_add_preprocess_rule)
	cmake_parse_arguments(_lc "" "ABS;DISPLAY;ID;FLAGS_FILE;COUNTS_DIR;IS_MSVC;CXX_STD;MODULE;SOURCE_ROOT;STRIP_PREFIX" "" ${ARGN})
	set(_count "${_lc_COUNTS_DIR}/${_lc_ID}.count")
	# For headers we may pass a stub path as ABS that already exists; for .cpp ABS is the source.
	add_custom_command(
		OUTPUT "${_count}"
		COMMAND ${CMAKE_COMMAND}
			"-DLINECOUNT_MODE=one"
			"-DLINECOUNT_COMPILER=${CMAKE_CXX_COMPILER}"
			"-DLINECOUNT_IS_MSVC=${_lc_IS_MSVC}"
			"-DLINECOUNT_CXX_STD=${_lc_CXX_STD}"
			"-DLINECOUNT_STUB=${_lc_ABS}"
			"-DLINECOUNT_COUNT_FILE=${_count}"
			"-DLINECOUNT_DISPLAY=${_lc_DISPLAY}"
			"-DLINECOUNT_FLAGS_FILE=${_lc_FLAGS_FILE}"
			-P "${_lc_MODULE}"
		DEPENDS "${_lc_ABS}" "${_lc_FLAGS_FILE}"
		COMMENT "linecount ${_lc_DISPLAY}"
		VERBATIM
	)
	set(_LINECOUNT_LAST_COUNT_FILE "${_count}" PARENT_SCOPE)
endfunction()

function(linecount_finalize)
	get_property(_done GLOBAL PROPERTY LINECOUNT_FINALIZED)
	if (_done)
		return()
	endif()
	set_property(GLOBAL PROPERTY LINECOUNT_FINALIZED TRUE)

	get_property(_targets GLOBAL PROPERTY LINECOUNT_TARGETS)
	if (NOT _targets)
		message(STATUS "linecount: no targets registered")
		return()
	endif()

	set(_lc_root "${CMAKE_BINARY_DIR}/linecount")
	set(_stubs_dir "${_lc_root}/stubs")
	set(_counts_dir "${_lc_root}/counts")
	set(_cpp_counts_dir "${_lc_root}/counts-cpp")
	set(_why_dir "${_lc_root}/why")
	set(_flags_dir "${_lc_root}/flags")
	file(MAKE_DIRECTORY "${_stubs_dir}")
	file(MAKE_DIRECTORY "${_counts_dir}")
	file(MAKE_DIRECTORY "${_cpp_counts_dir}")
	file(MAKE_DIRECTORY "${_why_dir}")
	file(MAKE_DIRECTORY "${_flags_dir}")

	set(_is_msvc FALSE)
	if (MSVC OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
		set(_is_msvc TRUE)
	endif()

	set(_cxx_std 17)
	if (CMAKE_CXX_STANDARD)
		set(_cxx_std "${CMAKE_CXX_STANDARD}")
	endif()

	set(_skip_target_re "${LINECOUNT_SKIP_TARGET_REGEX}")
	set(_skip_path_re "${LINECOUNT_SKIP_PATH_REGEX}")
	set(_module_file "${LINECOUNT_MODULE_FILE}")
	set(_source_root "${CMAKE_SOURCE_DIR}")
	if (LINECOUNT_SOURCE_ROOT)
		set(_source_root "${LINECOUNT_SOURCE_ROOT}")
	endif()
	set(_strip_prefix "${LINECOUNT_STRIP_PREFIX}")
	set(_why_top 40)
	if (LINECOUNT_WHY_TOP)
		set(_why_top "${LINECOUNT_WHY_TOP}")
	endif()

	set(_all_hdr_counts)
	set(_all_cpp_counts)
	set(_all_why_targets)

	foreach (_target ${_targets})
		if (NOT TARGET ${_target})
			continue()
		endif()
		if (_skip_target_re AND _target MATCHES "${_skip_target_re}")
			continue()
		endif()

		get_target_property(_srcdir ${_target} SOURCE_DIR)
		get_target_property(_srcs ${_target} SOURCES)
		if (NOT _srcs)
			continue()
		endif()

		set(_flags_file "${_flags_dir}/${_target}.flags")
		file(GENERATE
			OUTPUT "${_flags_file}"
			CONTENT
"$<$<BOOL:$<TARGET_PROPERTY:${_target},INCLUDE_DIRECTORIES>>:INCLUDE:$<JOIN:$<TARGET_PROPERTY:${_target},INCLUDE_DIRECTORIES>,\nINCLUDE:>\n>$<$<BOOL:$<TARGET_PROPERTY:${_target},COMPILE_DEFINITIONS>>:DEFINE:$<JOIN:$<TARGET_PROPERTY:${_target},COMPILE_DEFINITIONS>,\nDEFINE:>\n>"
		)

		set(_target_hdr_counts)
		set(_target_cpp_counts)

		foreach (_src ${_srcs})
			if (IS_ABSOLUTE "${_src}")
				set(_abs "${_src}")
			else()
				set(_abs "${_srcdir}/${_src}")
			endif()
			get_filename_component(_ext "${_abs}" EXT)
			string(TOLOWER "${_ext}" _ext_lower)

			file(RELATIVE_PATH _rel_full "${_source_root}" "${_abs}")
			if (_skip_path_re AND _rel_full MATCHES "${_skip_path_re}")
				continue()
			endif()

			_linecount_display_path("${_abs}" _display)
			string(MAKE_C_IDENTIFIER "${_target}_${_display}" _id)

			if (_ext_lower MATCHES "^\\.(h|hh|hpp|hxx|inl|inc)$")
				set(_stub "${_stubs_dir}/${_id}.cpp")
				file(WRITE "${_stub}" "#include \"${_abs}\"\n")
				_linecount_add_preprocess_rule(
					ABS "${_stub}"
					DISPLAY "${_display}"
					ID "${_id}"
					FLAGS_FILE "${_flags_file}"
					COUNTS_DIR "${_counts_dir}"
					IS_MSVC "${_is_msvc}"
					CXX_STD "${_cxx_std}"
					MODULE "${_module_file}"
				)
				list(APPEND _target_hdr_counts "${_LINECOUNT_LAST_COUNT_FILE}")
				list(APPEND _all_hdr_counts "${_LINECOUNT_LAST_COUNT_FILE}")

			elseif (_ext_lower MATCHES "^\\.(c|cc|cpp|cxx|c\\+\\+)$")
				_linecount_add_preprocess_rule(
					ABS "${_abs}"
					DISPLAY "${_display}"
					ID "${_id}"
					FLAGS_FILE "${_flags_file}"
					COUNTS_DIR "${_cpp_counts_dir}"
					IS_MSVC "${_is_msvc}"
					CXX_STD "${_cxx_std}"
					MODULE "${_module_file}"
				)
				list(APPEND _target_cpp_counts "${_LINECOUNT_LAST_COUNT_FILE}")
				list(APPEND _all_cpp_counts "${_LINECOUNT_LAST_COUNT_FILE}")

				# Per-TU include contribution breakdown.
				get_filename_component(_base "${_abs}" NAME_WE)
				string(MAKE_C_IDENTIFIER "${_base}" _base_id)
				set(_why_tgt "linecount-why-${_target}-${_base_id}")
				set(_why_report "${_why_dir}/${_target}-${_base_id}.txt")
				add_custom_command(
					OUTPUT "${_why_report}"
					COMMAND ${CMAKE_COMMAND}
						"-DLINECOUNT_MODE=why"
						"-DLINECOUNT_COMPILER=${CMAKE_CXX_COMPILER}"
						"-DLINECOUNT_IS_MSVC=${_is_msvc}"
						"-DLINECOUNT_CXX_STD=${_cxx_std}"
						"-DLINECOUNT_SOURCE=${_abs}"
						"-DLINECOUNT_REPORT=${_why_report}"
						"-DLINECOUNT_FLAGS_FILE=${_flags_file}"
						"-DLINECOUNT_SOURCE_ROOT=${_source_root}"
						"-DLINECOUNT_STRIP_PREFIX=${_strip_prefix}"
						"-DLINECOUNT_TOP=${_why_top}"
						-P "${_module_file}"
					DEPENDS "${_abs}" "${_flags_file}"
					COMMENT "linecount-why ${_display}"
					VERBATIM
				)
				# Always print when the target is requested (even if report is up to date).
				add_custom_target(${_why_tgt}
					DEPENDS "${_why_report}"
					COMMAND ${CMAKE_COMMAND}
						"-DLINECOUNT_MODE=show"
						"-DLINECOUNT_REPORT=${_why_report}"
						-P "${_module_file}"
					USES_TERMINAL
					VERBATIM
				)
				list(APPEND _all_why_targets ${_why_tgt})
			endif()
		endforeach()

		if (_target_hdr_counts)
			set(_list_file "${_counts_dir}/${_target}.list")
			string(REPLACE ";" "\n" _list_body "${_target_hdr_counts}")
			file(WRITE "${_list_file}" "${_list_body}\n")
			add_custom_target(linecount-${_target}
				DEPENDS ${_target_hdr_counts}
				COMMAND ${CMAKE_COMMAND}
					"-DLINECOUNT_MODE=report"
					"-DLINECOUNT_REPORT=${_lc_root}/report-${_target}.txt"
					"-DLINECOUNT_FILTER_LIST=${_list_file}"
					-P "${_module_file}"
				COMMENT "Header line counts for ${_target}"
				USES_TERMINAL
				VERBATIM
			)
		endif()

		if (_target_cpp_counts)
			set(_list_file "${_cpp_counts_dir}/${_target}.list")
			string(REPLACE ";" "\n" _list_body "${_target_cpp_counts}")
			file(WRITE "${_list_file}" "${_list_body}\n")
			add_custom_target(linecount-cpp-${_target}
				DEPENDS ${_target_cpp_counts}
				COMMAND ${CMAKE_COMMAND}
					"-DLINECOUNT_MODE=report"
					"-DLINECOUNT_REPORT=${_lc_root}/report-cpp-${_target}.txt"
					"-DLINECOUNT_FILTER_LIST=${_list_file}"
					-P "${_module_file}"
				COMMENT "TU line counts for ${_target}"
				USES_TERMINAL
				VERBATIM
			)
		endif()
	endforeach()

	if (_all_hdr_counts)
		add_custom_target(linecount
			DEPENDS ${_all_hdr_counts}
			COMMAND ${CMAKE_COMMAND}
				"-DLINECOUNT_MODE=report"
				"-DLINECOUNT_COUNTS_DIR=${_counts_dir}"
				"-DLINECOUNT_REPORT=${_lc_root}/report.txt"
				-P "${_module_file}"
			COMMENT "Header line counts (path:lines, sorted)"
			USES_TERMINAL
			VERBATIM
		)
	else()
		add_custom_target(linecount
			COMMAND ${CMAKE_COMMAND} -E echo "linecount: no headers found"
		)
	endif()

	if (_all_cpp_counts)
		add_custom_target(linecount-cpp
			DEPENDS ${_all_cpp_counts}
			COMMAND ${CMAKE_COMMAND}
				"-DLINECOUNT_MODE=report"
				"-DLINECOUNT_COUNTS_DIR=${_cpp_counts_dir}"
				"-DLINECOUNT_REPORT=${_lc_root}/report-cpp.txt"
				-P "${_module_file}"
			COMMENT "TU line counts (path:lines, sorted)"
			USES_TERMINAL
			VERBATIM
		)
	else()
		add_custom_target(linecount-cpp
			COMMAND ${CMAKE_COMMAND} -E echo "linecount-cpp: no sources found"
		)
	endif()

	list(LENGTH _all_hdr_counts _hdr_n)
	list(LENGTH _all_cpp_counts _cpp_n)
	list(LENGTH _all_why_targets _why_n)
	message(STATUS "linecount: ${_hdr_n} headers, ${_cpp_n} TUs, ${_why_n} why-targets (no link)")
endfunction()

if (LINECOUNT_AUTO_FINALIZE AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
	cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL linecount_finalize)
endif()
