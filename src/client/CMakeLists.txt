fips_begin_app(client windowed)
	fips_files(
		Client.h Client.cpp
	)

	fips_dir(sauce)
	fips_files(
		ClientModule.h
		ClientInjector.h
	)

	fips_dir(network)
	fips_files(
		AuthFailedHandler.h
		ClientNetworkModule.h
		NpcSpawnHandler.h
		SeedHandler.cpp SeedHandler.h
		UserSpawnHandler.h
		UserUpdateHandler.h
		NpcRemoveHandler.h
	)
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS} .)
	fips_deps(frontend util ${SDL2_LIBRARIES})
	copy_data_files(client)
fips_end_app()
