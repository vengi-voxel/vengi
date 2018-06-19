include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

set(DEFAULT_LUA_EXECUTABLE lua lua5.2 lua5.3 luac luac5.2 luac5.3)
set(GAME_BASE_DIR data CACHE STRING "" FORCE)

macro(copy_data_files TARGET)
	add_custom_target(copy-data-${TARGET} ALL
		COMMAND cmake -E copy_directory "${ROOT_DIR}/${GAME_BASE_DIR}/${TARGET}/" ${CMAKE_BINARY_DIR}
		COMMENT "Copy ${TARGET} data files...")
endmacro()

macro(check_lua_files TARGET)
	set(files ${ARGV})
	list(REMOVE_AT files 0)
	find_program(LUA_EXECUTABLE NAMES ${DEFAULT_LUA_EXECUTABLE})
	if (LUA_EXECUTABLE)
		message("${LUA_EXECUTABLE} found")
		foreach(_file ${files})
			string(REGEX REPLACE "[/]" "_" targetname ${_file})
			add_custom_target(
				${targetname}
				COMMAND ${LUA_EXECUTABLE} ${_file}
				COMMENT "Validate ${_file}"
				WORKING_DIRECTORY ${ROOT_DIR}/${GAME_BASE_DIR}/${TARGET}
			)
			add_dependencies(${TARGET} ${targetname})
		endforeach()
	else()
		foreach(_file ${files})
			string(REGEX REPLACE "[/]" "_" targetname ${_file})
			add_custom_target(
				${targetname}
				COMMAND ${CMAKE_BINARY_DIR}/luac ${_file}
				COMMENT "Validate ${_file}"
				DEPENDS luac
				WORKING_DIRECTORY ${ROOT_DIR}/${GAME_BASE_DIR}/${TARGET}
			)
			add_dependencies(${TARGET} ${targetname})
		endforeach()
	endif()
endmacro()

macro(check_ui_turbobadger TARGET)
	set(_workingdir "${ROOT_DIR}/${GAME_BASE_DIR}/${TARGET}")
	set(_dir "${_workingdir}/ui/window")
	file(GLOB UI_FILES ${_dir}/*.tb.txt)
	foreach(_file ${UI_FILES})
		get_filename_component(_filename ${_file} NAME)
		add_custom_target(
			${_filename}
			COMMAND ${CMAKE_BINARY_DIR}/uitool ui/window/${_filename}
			COMMENT "Validate ui file: ${_filename}"
			DEPENDS uitool
			WORKING_DIRECTORY ${_workingdir}
		)
		add_dependencies(${TARGET} ${_filename})
	endforeach()
	if (UI_FILES)
		add_dependencies(${TARGET} uitool)
	endif()
endmacro()

include(CheckCCompilerFlag)

check_cxx_compiler_flag("-std=c++14" COMPILER_SUPPORTS_CXX14)
if (COMPILER_SUPPORTS_CXX14)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
elseif (NOT MSVC)
	# Don't error out, it might still work
	message(SEND_ERROR "It looks like your compiler doesn't understand -std=c++14")
endif()
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

find_program(DESKTOP_FILE_VALIDATE_EXECUTABLE desktop-file-validate)

if (USE_GPROF)
	check_cxx_compiler_flag("-pg" COMPILER_SUPPORTS_GNUPROF)
	if (COMPILER_SUPPORTS_GNUPROF)
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
	endif()
endif()

check_cxx_compiler_flag("-Wthread-safety" HAVE_THREAD_SAFETY_CHECKS)
if (HAVE_THREAD_SAFETY_CHECKS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wthread-safety")
endif()
check_cxx_compiler_flag("-fdiagnostics-parseable-fixits" CXX_COMPILER_SUPPORTS_FIXITS)
if (CXX_COMPILER_SUPPORTS_FIXITS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-parseable-fixits")
endif()
check_c_compiler_flag("-fdiagnostics-parseable-fixits" C_COMPILER_SUPPORTS_FIXITS)
if (C_COMPILER_SUPPORTS_FIXITS)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-parseable-fixits")
endif()

if (USE_DOXYGEN_CHECK)
	check_cxx_compiler_flag("-Wdocumentation" COMPILER_SUPPORTS_WDOCUMENTATION)
	if (COMPILER_SUPPORTS_WDOCUMENTATION)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdocumentation")
	endif()
endif()

# thread sanitizer doesn't work in combination with address and leak

# Set -Werror to catch "argument unused during compilation" warnings
set(CMAKE_REQUIRED_FLAGS "-Werror -fthread-sanitizer") # Also needs to be a link flag for test to pass
check_c_compiler_flag("-fthread-sanitizer" HAVE_FLAG_THREAD_SANITIZER)
set(CMAKE_REQUIRED_FLAGS "-Werror -fsanitize=thread") # Also needs to be a link flag for test to pass
check_c_compiler_flag("-fsanitize=thread" HAVE_FLAG_SANITIZE_THREAD)
set(CMAKE_REQUIRED_FLAGS "-Werror")
check_c_compiler_flag("-fsanitize=undefined" HAVE_FLAG_SANITIZE_UNDEFINED)
check_c_compiler_flag("-fsanitize=address" HAVE_FLAG_SANITIZE_ADDRESS)
check_c_compiler_flag("-fsanitize=leak" HAVE_FLAG_SANITIZE_LEAK)
check_c_compiler_flag("-fexpensive-optimizations" HAVE_EXPENSIVE_OPTIMIZATIONS)
unset(CMAKE_REQUIRED_FLAGS)

#-Wthread-safety - http://clang.llvm.org/docs/ThreadSafetyAnalysis.html

if (HAVE_FLAG_SANITIZE_UNDEFINED)
	set(SANITIZE_UNDEFINED_FLAG "-fsanitize=undefined" CACHE STRING "" FORCE)
	message("Support undefined sanitizer")
endif()

if (HAVE_FLAG_SANITIZE_LEAK)
	set(SANITIZE_LEAK_FLAG "-fsanitize=leak" CACHE STRING "" FORCE)
	message("Support leak sanitizer")
endif()

if (HAVE_FLAG_SANITIZE_THREAD)
	set(SANITIZE_THREAD_FLAG "-fsanitize=thread" CACHE STRING "" FORCE)
	message("Support thread sanitizer")
endif()

if (HAVE_FLAG_THREAD_SANITIZER)
	set(SANITIZE_THREAD_FLAG "-fthread-sanitizer" CACHE STRING "" FORCE)
	message("Support thread sanitizer")
endif()

if (HAVE_FLAG_SANITIZE_ADDRESS)
	set(SANITIZE_ADDRESS_FLAG "-fsanitize=address" CACHE STRING "" FORCE)
	message("Support address sanitizer")
endif()
