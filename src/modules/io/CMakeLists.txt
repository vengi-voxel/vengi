fips_begin_module(io)
	fips_files(
		EventHandler.cpp EventHandler.h IEventObserver.h
		Filesystem.cpp Filesystem.h
		File.cpp File.h
		IOResource.h
	)
	fips_deps(sdl2)
fips_end_module()
