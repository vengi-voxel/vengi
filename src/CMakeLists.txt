if (USE_LINK_WHAT_YOU_USE)
	set(CMAKE_LINK_WHAT_YOU_USE ON)
endif()

if (USE_CLANG_TIDY)
find_host_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy clang-tidy-3.8 clang-tidy-8 clang-tidy-9 clang-tidy-10 clang-tidy-11)
if (CLANG_TIDY_EXECUTABLE)
	set(CMAKE_CXX_CLANG_TIDY
		${CLANG_TIDY_EXECUTABLE};
		-fix;
)
endif()
endif()

if (USE_CPPCHECK)
find_host_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
list(APPEND CMAKE_CXX_CPPCHECK
	"--enable=all"
	"--inconclusive"
	"--force"
	"--inline-suppr"
	"--suppressions-list=${SCRIPTS_CMAKE_DIR}/CppCheckSuppressions.txt"
)
else()
set(CMAKE_CXX_CPPCHECK "")
endif()

add_subdirectory(modules)
if (CLIENT)
	add_subdirectory(client)
endif()
if (SERVER)
	add_subdirectory(server)
endif()
add_subdirectory(tools)
if (VISUALTESTS)
	add_subdirectory(tests)
endif()
