set(SRCS
	File.cpp File.h
	FileStream.cpp FileStream.h
	Filesystem.cpp Filesystem.h
	IOResource.h
)

set(LIB io)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core)

set(TEST_FILES
	testio/iotest.txt
)

set(TEST_SRCS
	tests/FilesystemTest.cpp
	tests/FileStreamTest.cpp
	tests/FileTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app)
gtest_suite_files(tests ${TEST_FILES})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_end(tests-${LIB})
