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
	fips_dir(ui)
	fips_files(
		LoginWindow.h
		HudWindow.h
		AuthFailedWindow.h
		DisconnectWindow.h
	)
	fips_deps(frontend util)
	copy_data_files(client)
fips_end_app()
check_glsl_files(client world mesh ui)
