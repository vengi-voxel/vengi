set(SRCS
	external/miniz.h external/miniz.c

	Archive.cpp Archive.h
	BufferedReadWriteStream.cpp BufferedReadWriteStream.h
	BufferedSeekableWriteStream.h
	BufferedWriteStream.h
	File.cpp File.h
	FileStream.cpp FileStream.h
	Filesystem.cpp Filesystem.h
	FilesystemArchive.cpp FilesystemArchive.h
	FilesystemEntry.h
	FormatDescription.cpp FormatDescription.h
	IOResource.h
	LZFSEReadStream.cpp LZFSEReadStream.h
	MemoryReadStream.cpp MemoryReadStream.h
	StdStreamBuf.h
	Stream.cpp Stream.h
	StringStream.cpp StringStream.h
	ZipArchive.cpp ZipArchive.h
	ZipReadStream.cpp ZipReadStream.h
	ZipWriteStream.cpp ZipWriteStream.h

	system/Null.cpp
	system/Unix.cpp
	system/Windows.cpp
)

set(LIB io)
set(LIBS core lzfse)
#find_package(ZLIB)
set(ZLIB_FOUND FALSE)
if (ZLIB_FOUND)
  list(APPEND LIBS ZLIB::ZLIB)
endif()
set(USE_ZLIB ${ZLIB_FOUND} CACHE BOOL "Use zlib for zip compression" FORCE)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})

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
