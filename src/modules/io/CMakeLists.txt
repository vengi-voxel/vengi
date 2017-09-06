set(SRCS
	EventHandler.cpp EventHandler.h IEventObserver.h
	Filesystem.cpp Filesystem.h
	FileStream.cpp FileStream.h
	File.cpp File.h
	IOResource.h
)

if (LINUX OR DARWIN)
	list(APPEND SRCS FilesystemUnix.cpp)
elseif (WINDOWS)
	list(APPEND SRCS FilesystemWindows.cpp)
endif()

set(LIB io)
add_library(${LIB} ${SRCS})

engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES sdl2 easy_profiler glm backward libuv)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/FilesystemTest.cpp
	tests/FileStreamTest.cpp
	tests/FileTest.cpp
)
gtest_suite_deps(tests ${LIB})
