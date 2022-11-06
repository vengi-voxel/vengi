if (USE_CLANG_TIDY)
find_host_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy clang-tidy-3.8 clang-tidy-8 clang-tidy-9 clang-tidy-10 clang-tidy-11)
if (CLANG_TIDY_EXECUTABLE)
	set(CMAKE_CXX_CLANG_TIDY
		${CLANG_TIDY_EXECUTABLE};
		-fix-errors;
)
endif()
endif()

add_subdirectory(modules)
add_subdirectory(tools)
if (VISUALTESTS)
	add_subdirectory(tests)
endif()
