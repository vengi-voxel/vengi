find_package(Git QUIET)

set(GIT_COMMIT "-")
set(GIT_COMMIT_DATE "-")

if(GIT_FOUND)
	execute_process(
		COMMAND
		${GIT_EXECUTABLE} show -s "--format=%h;%cd"
		WORKING_DIRECTORY
		${ROOT_DIR}
		OUTPUT_VARIABLE
		git_show_output
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET
	)

	if(git_show_output)
		list(GET git_show_output 0 GIT_COMMIT)
		list(GET git_show_output 1 GIT_COMMIT_DATE)
	endif()
endif()

set(GIT_COMMIT ${GIT_COMMIT} CACHE STRING "" FORCE)
set(GIT_COMMIT_DATE ${GIT_COMMIT_DATE} CACHE STRING "" FORCE)
