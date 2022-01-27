set(SRCS
	BufferedReadWriteStream.cpp BufferedReadWriteStream.h
	File.cpp File.h
	FileStream.cpp FileStream.h
	Filesystem.cpp Filesystem.h
	FormatDescription.cpp FormatDescription.h
	IOResource.h
	Stream.cpp Stream.h
	MemoryReadStream.cpp MemoryReadStream.h
	BufferedZipReadStream.cpp BufferedZipReadStream.h
	ZipWriteStream.h ZipWriteStream.cpp
	ZipReadStream.h ZipReadStream.cpp
)

set(LIB io)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core libuv)

set(TEST_FILES
	testio/iotest.txt
)

set(TEST_SRCS
	tests/BufferedReadWriteStreamTest.cpp
	tests/FilesystemTest.cpp
	tests/FileStreamTest.cpp
	tests/FormatDescriptionTest.cpp
	tests/FileTest.cpp
	tests/MemoryReadStreamTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app)
gtest_suite_files(tests ${TEST_FILES})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_end(tests-${LIB})
