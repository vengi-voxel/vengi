set(SRCS
	EventHandler.cpp EventHandler.h IEventObserver.h
	Filesystem.cpp Filesystem.h
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
# TODO: pick one di framework
target_link_libraries(${LIB} sdl2 glm backward)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/FilesystemTest.cpp
	tests/FileTest.cpp
)
gtest_suite_deps(tests ${LIB})
