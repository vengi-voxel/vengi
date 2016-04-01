fips_begin_module(io)
	fips_files(
		EventHandler.cpp EventHandler.h IEventObserver.h
		Filesystem.cpp Filesystem.h
		File.cpp File.h
		IOResource.h
	)
fips_end_module()

find_package(SDL2 REQUIRED)
target_link_libraries(io ${SDL2_LIBRARIES})
target_include_directories(io PUBLIC ${SDL2_INCLUDE_DIRS})
