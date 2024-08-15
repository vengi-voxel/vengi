set(SRCS
	Downloader.h Downloader.cpp
	GithubAPI.h GithubAPI.cpp
	GitlabAPI.h GitlabAPI.cpp
	CollectionManager.h CollectionManager.cpp
)

set(LIB voxelcollection)
set(DEPENDENCIES http json voxelformat voxelrender)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${DEPENDENCIES})

set(TEST_SRCS
	tests/CollectionManagerTest.cpp
	tests/DownloaderTest.cpp
	tests/GithubAPITest.cpp
	tests/GitlabAPITest.cpp
)
set(TEST_FILES
	tests/ambient-occlusion.vengi
)
gtest_suite_sources(tests
	${TEST_SRCS}
)
gtest_suite_deps(tests ${LIB} test-app)
gtest_suite_files(tests ${TEST_FILES})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
