set(SRCS
	Archive.cpp Archive.h
	BufferedReadWriteStream.cpp BufferedReadWriteStream.h
	File.cpp File.h
	FileStream.cpp FileStream.h
	Filesystem.cpp Filesystem.h
	FilesystemArchive.cpp FilesystemArchive.h
	FormatDescription.cpp FormatDescription.h
	IOResource.h
	StdStreamBuf.h
	Stream.cpp Stream.h
	LZFSEReadStream.h LZFSEReadStream.cpp
	MemoryReadStream.cpp MemoryReadStream.h
	BufferedWriteStream.h
	BufferedSeekableWriteStream.h
	BufferedZipReadStream.cpp BufferedZipReadStream.h
	StringStream.cpp StringStream.h
	ZipArchive.h ZipArchive.cpp
	ZipWriteStream.h ZipWriteStream.cpp
	ZipReadStream.h ZipReadStream.cpp
	system/Null.cpp
	system/Unix.cpp
	system/Windows.cpp
)

set(LIB io)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core lzfse)

set(TEST_FILES
	testio/iotest.txt
	testio/iotest.zip
)

set(TEST_SRCS
	tests/BufferedReadWriteStreamTest.cpp
	tests/BufferedWriteStreamTest.cpp
	tests/FilesystemTest.cpp
	tests/FileStreamTest.cpp
	tests/FormatDescriptionTest.cpp
	tests/FileTest.cpp
	tests/MemoryReadStreamTest.cpp
	tests/StdStreamBufTest.cpp
	tests/ZipArchiveTest.cpp
	tests/ZipStreamTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_end(tests-${LIB})
