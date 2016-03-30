SET(DEFAULT_LUA_EXECUTABLE lua)

macro(copy_data_files TARGET)
	add_custom_target(copy-data-${TARGET} ALL
		COMMAND cmake -E copy_directory "${FIPS_PROJECT_DIR}/data/${TARGET}/" ${FIPS_DEPLOY_DIR}/${CMAKE_PROJECT_NAME}/${FIPS_CONFIG}
		COMMENT "Copy ${TARGET} data files...")
endmacro()

macro(check_lua_files TARGET files)
	find_program(LUA_EXECUTABLE NAMES ${DEFAULT_LUA_EXECUTABLE})
	if (LUA_EXECUTABLE)
		message("${LUA_EXECUTABLE} found")
		foreach(_file ${files})
			add_custom_target(
				${_file}
				COMMAND ${LUA_EXECUTABLE} ${_file}
				COMMENT "Validate ${_file}"
				WORKING_DIRECTORY ${FIPS_PROJECT_DIR}/data/${TARGET}
			)
			add_dependencies(${TARGET} ${_file})
		endforeach()
	else()
		message(WARNING "No ${DEFAULT_LUA_EXECUTABLE} found")
	endif()
endmacro()

include(CheckCCompilerFlag)

# thread sanitizer doesn't work in combination with address and leak
set(CMAKE_REQUIRED_FLAGS "-Werror -fsanitize=thread")
check_c_compiler_flag("-fsanitize=thread" HAVE_FLAG_SANITIZE_THREAD)
set(CMAKE_REQUIRED_FLAGS "-Werror -fsanitize=undefined")
check_c_compiler_flag("-fsanitize=undefined" HAVE_FLAG_SANITIZE_UNDEFINED)
#set(CMAKE_REQUIRED_FLAGS "-Werror -fsanitize=address")
#check_c_compiler_flag("-fsanitize=address" HAVE_FLAG_SANITIZE_ADDRESS)
#set(CMAKE_REQUIRED_FLAGS "-Werror -fsanitize=leak")
#check_c_compiler_flag("-fsanitize=leak" HAVE_FLAG_SANITIZE_LEAK)
set(CMAKE_REQUIRED_FLAGS "-Werror -fexpensive-optimizations")
check_c_compiler_flag("-fexpensive-optimizations" HAVE_EXPENSIVE_OPTIMIZATIONS)
unset(CMAKE_REQUIRED_FLAGS)

#-Wthread-safety - http://clang.llvm.org/docs/ThreadSafetyAnalysis.html

if (HAVE_FLAG_SANITIZE_UNDEFINED)
	set(SANITIZE_FLAGS "${SANITIZE_FLAGS} -fsanitize=undefined")
	message("Support undefined sanitizer")
endif()

if (HAVE_FLAG_SANITIZE_LEAK)
	set(SANITIZE_FLAGS "${SANITIZE_FLAGS} -fsanitize=leak")
	message("Support leak sanitizer")
endif()

if (HAVE_FLAG_SANITIZE_THREAD)
	set(SANITIZE_FLAGS "${SANITIZE_FLAGS} -fsanitize=thread")
	message("Support thread sanitizer")
endif()

if (HAVE_FLAG_SANITIZE_ADDRESS)
	set(SANITIZE_FLAGS "${SANITIZE_FLAGS} -fsanitize=address")
	message("Support address sanitizer")
endif()
