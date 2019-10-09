set(SRCS
	EventHandler.cpp EventHandler.h
	File.cpp File.h
	FileStream.cpp FileStream.h
	Filesystem.cpp Filesystem.h
	IEventObserver.h
	IOResource.h
)

set(LIB io)
set(LIBS sdl2 glm libuv)
if (NOT MSVC)
list(APPEND LIBS backward)
endif()
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})

set(TEST_SRCS
	tests/FilesystemTest.cpp
	tests/FileStreamTest.cpp
	tests/FileTest.cpp
)

gtest_suite_files(tests tests/iotest.txt)
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_files(tests-${LIB} tests/iotest.txt)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
