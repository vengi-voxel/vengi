include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

set(DEFAULT_LUA_EXECUTABLE lua lua5.2 lua5.3)

macro(copy_data_files TARGET)
	add_custom_target(copy-data-${TARGET} ALL
		COMMAND cmake -E copy_directory "${ROOT_DIR}/data/${TARGET}/" ${CMAKE_BINARY_DIR}
		COMMENT "Copy ${TARGET} data files...")
endmacro()

macro(check_lua_files TARGET)
	set(files ${ARGV})
	list(REMOVE_AT files 0)
	find_program(LUA_EXECUTABLE NAMES ${DEFAULT_LUA_EXECUTABLE})
	if (LUA_EXECUTABLE)
		message("${LUA_EXECUTABLE} found")
		foreach(_file ${files})
			add_custom_target(
				${_file}
				COMMAND ${LUA_EXECUTABLE} ${_file}
				COMMENT "Validate ${_file}"
				WORKING_DIRECTORY ${ROOT_DIR}/data/${TARGET}
			)
			add_dependencies(${TARGET} ${_file})
		endforeach()
	else()
		message(WARNING "No ${DEFAULT_LUA_EXECUTABLE} found")
	endif()
endmacro()

include(CheckCCompilerFlag)

check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if (COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif (COMPILER_SUPPORTS_CXX0X)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# thread sanitizer doesn't work in combination with address and leak
set(CMAKE_REQUIRED_FLAGS "-Werror -fsanitize=thread")
check_c_compiler_flag("-fsanitize=thread" HAVE_FLAG_SANITIZE_THREAD)
set(CMAKE_REQUIRED_FLAGS "-Werror -fsanitize=undefined")
check_c_compiler_flag("-fsanitize=undefined" HAVE_FLAG_SANITIZE_UNDEFINED)
set(CMAKE_REQUIRED_FLAGS "-Werror -fsanitize=address")
check_c_compiler_flag("-fsanitize=address" HAVE_FLAG_SANITIZE_ADDRESS)
set(CMAKE_REQUIRED_FLAGS "-Werror -fsanitize=leak")
check_c_compiler_flag("-fsanitize=leak" HAVE_FLAG_SANITIZE_LEAK)
set(CMAKE_REQUIRED_FLAGS "-Werror -fexpensive-optimizations")
check_c_compiler_flag("-fexpensive-optimizations" HAVE_EXPENSIVE_OPTIMIZATIONS)
unset(CMAKE_REQUIRED_FLAGS)

#-Wthread-safety - http://clang.llvm.org/docs/ThreadSafetyAnalysis.html

if (HAVE_FLAG_SANITIZE_UNDEFINED)
#	set(SANITIZE_FLAGS "${SANITIZE_FLAGS} -fsanitize=undefined")
	message("Support undefined sanitizer")
endif()

if (HAVE_FLAG_SANITIZE_LEAK)
#	set(SANITIZE_FLAGS "${SANITIZE_FLAGS} -fsanitize=leak")
	message("Support leak sanitizer")
endif()

if (HAVE_FLAG_SANITIZE_THREAD)
	set(SANITIZE_FLAGS "${SANITIZE_FLAGS} -fsanitize=thread")
	message("Support thread sanitizer")
endif()

if (HAVE_FLAG_SANITIZE_ADDRESS)
#	set(SANITIZE_FLAGS "${SANITIZE_FLAGS} -fsanitize=address")
	message("Support address sanitizer")
endif()
