fips_begin_module(io)
	fips_files(
		EventHandler.cpp EventHandler.h IEventObserver.h
		Filesystem.cpp Filesystem.h
		File.cpp File.h
		IOResource.h
	)
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS})
	fips_deps(${SDL2_LIBRARIES})
fips_end_module()
