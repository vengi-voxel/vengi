set(SRCS
	external/miniz.h external/miniz.c

	Archive.cpp Archive.h
	Base64.cpp Base64.h
	Base64Stream.h
	Base64ReadStream.cpp Base64ReadStream.h
	Base64WriteStream.cpp Base64WriteStream.h
	BufferedReadWriteStream.cpp BufferedReadWriteStream.h
	BufferedSeekableWriteStream.h
	BufferedWriteStream.h
	EndianStreamReadWrapper.h
	File.cpp File.h
	FileStream.cpp FileStream.h
	Filesystem.cpp Filesystem.h
	FilesystemArchive.cpp FilesystemArchive.h
	FilesystemEntry.cpp FilesystemEntry.h
	FormatDescription.cpp FormatDescription.h
	IOResource.h
	LZFSEReadStream.cpp LZFSEReadStream.h
	MemoryArchive.cpp MemoryArchive.h
	MemoryReadStream.cpp MemoryReadStream.h
	StdStreamBuf.h
	StderrWriteStream.h
	StdoutWriteStream.h
	Stream.cpp Stream.h
	StreamUtil.h
	StringStream.cpp StringStream.h
	TokenStream.cpp TokenStream.h
	ZipArchive.cpp ZipArchive.h
	ZipReadStream.cpp ZipReadStream.h
	ZipWriteStream.cpp ZipWriteStream.h
	Z85.h Z85.cpp

	system/System.h
	system/Null.cpp
	system/Unix.cpp
	system/Windows.cpp
	system/emscripten_browser_file.h
	system/windirent.h
)

set(LIB io)
set(LIBS core lzfse)

find_package(libdeflate)
if (libdeflate_FOUND)
	if (TARGET libdeflate::libdeflate)
		list(APPEND LIBS libdeflate::libdeflate)
	elseif (DEFINED LIBDEFLATE_LIBRARIES)
		list(APPEND LIBS ${LIBDEFLATE_LIBRARIES})
	elseif (DEFINED LIBDEFLATE_LIBRARY)
		list(APPEND LIBS ${LIBDEFLATE_LIBRARY})
	else()
		find_package(PkgConfig QUIET)
		if (PkgConfig_FOUND)
			pkg_check_modules(PC_LIBDEFLATE libdeflate)
			if (PC_LIBDEFLATE_FOUND)
				list(APPEND LIBS ${PC_LIBDEFLATE_LIBRARIES})
				include_directories(${PC_LIBDEFLATE_INCLUDE_DIRS})
			else()
				set(libdeflate_FOUND FALSE)
			endif()
		else()
			set(libdeflate_FOUND FALSE)
		endif()
	endif()
endif()
set(USE_LIBDEFLATE ${libdeflate_FOUND} CACHE BOOL "Use libdeflate for zip compression" FORCE)

#find_package(ZLIB)
set(ZLIB_FOUND FALSE)
if (ZLIB_FOUND)
  list(APPEND LIBS ZLIB::ZLIB)
endif()
set(USE_ZLIB ${ZLIB_FOUND} CACHE BOOL "Use zlib for zip compression" FORCE)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})
target_compile_definitions(${LIB} PRIVATE MINIZ_NO_STDIO)

set(TEST_FILES
	testio/iotest.txt
	testio/iotest.zip
)

set(TEST_SRCS
	tests/Base64Test.cpp
	tests/BufferedReadWriteStreamTest.cpp
	tests/BufferedWriteStreamTest.cpp
	tests/FilesystemTest.cpp
	tests/FilesystemArchiveTest.cpp
	tests/FileStreamTest.cpp
	tests/FormatDescriptionTest.cpp
	tests/FileTest.cpp
	tests/MemoryArchiveTest.cpp
	tests/MemoryReadStreamTest.cpp
	tests/StdStreamBufTest.cpp
	tests/TokenStreamTest.cpp
	tests/ZipArchiveTest.cpp
	tests/ZipStreamTest.cpp
	tests/Z85Test.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} test-app)
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_end(tests-${LIB})

set(BENCHMARK_SRCS
	benchmarks/StreamBenchmark.cpp
)
engine_add_executable(TARGET benchmarks-${LIB} SRCS ${BENCHMARK_SRCS} NOINSTALL)
engine_target_link_libraries(TARGET benchmarks-${LIB} DEPENDENCIES benchmark-app ${LIB})
