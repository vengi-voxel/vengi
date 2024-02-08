set(SRCS
	Downloader.h Downloader.cpp
	GithubAPI.h GithubAPI.cpp
	JsonUtil.h JsonUtil.cpp
)

set(LIB voxbrowser-util)
set(DEPENDENCIES http json voxelformat)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${DEPENDENCIES})

set(TEST_SRCS
	tests/DownloaderTest.cpp
	tests/GithubAPITest.cpp
)

gtest_suite_sources(tests
	${TEST_SRCS}
)
gtest_suite_deps(tests ${LIB} test-app)
gtest_suite_files(tests ${TEST_FILES})

if (NOT CMAKE_GENERATOR MATCHES "Xcode")
	gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
	gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
	gtest_suite_deps(tests-${LIB} ${LIB} test-app)
	gtest_suite_end(tests-${LIB})
endif()
