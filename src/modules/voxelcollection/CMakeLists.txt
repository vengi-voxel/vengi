set(SRCS
	Downloader.h Downloader.cpp
	CubZHAPI.h CubZHAPI.cpp
	GithubAPI.h GithubAPI.cpp
	GitlabAPI.h GitlabAPI.cpp
	CollectionManager.h CollectionManager.cpp
	ui/CollectionPanel.h ui/CollectionPanel.cpp
)

set(LIB voxelcollection)
set(DEPENDENCIES http json voxelformat ui)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${DEPENDENCIES})

set(TEST_SRCS
	tests/CollectionManagerTest.cpp
	tests/DownloaderTest.cpp
	tests/GithubAPITest.cpp
	tests/GitlabAPITest.cpp
	tests/CubZHAPITest.cpp
)
set(TEST_FILES
	tests/ambient-occlusion.vengi
)
gtest_suite_sources(tests
	${TEST_SRCS}
)
gtest_suite_deps(tests ${LIB} test-app)
gtest_suite_files(tests ${TEST_FILES})

if (NOT CMAKE_GENERATOR MATCHES "Xcode")
	gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
	gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
	gtest_suite_files(tests-${LIB} ${TEST_FILES})
	gtest_suite_deps(tests-${LIB} ${LIB} test-app)
	gtest_suite_end(tests-${LIB})
endif()
