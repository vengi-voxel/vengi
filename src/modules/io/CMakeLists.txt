set(SRCS
	EventHandler.cpp EventHandler.h
	File.cpp File.h
	FileStream.cpp FileStream.h
	Filesystem.cpp Filesystem.h
	IEventObserver.h
	IOResource.h
)

set(LIB io)
add_library(${LIB} ${SRCS})

engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES sdl2 glm backward libuv)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/FilesystemTest.cpp
	tests/FileStreamTest.cpp
	tests/FileTest.cpp
)
gtest_suite_deps(tests ${LIB})
