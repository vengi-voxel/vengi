set(SRCS
	EventHandler.cpp EventHandler.h
	File.cpp File.h
	FileStream.cpp FileStream.h
	Filesystem.cpp Filesystem.h
	IEventObserver.h
	IOResource.h
)

set(LIB io)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES sdl2 glm backward libuv)

gtest_suite_sources(tests
	tests/FilesystemTest.cpp
	tests/FileStreamTest.cpp
	tests/FileTest.cpp
)
gtest_suite_files(tests tests/iotest.txt)
gtest_suite_deps(tests ${LIB})
