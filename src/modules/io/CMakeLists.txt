set(SRCS
	io/EventHandler.cpp io/EventHandler.h
	io/File.cpp io/File.h
	io/FileStream.cpp io/FileStream.h
	io/Filesystem.cpp io/Filesystem.h
	io/IEventObserver.h
	io/IOResource.h
)

set(LIB io)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core)

set(TEST_FILES
	testapp/iotest.txt
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
