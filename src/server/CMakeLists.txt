fips_begin_app(server cmdline)
	fips_files(
		Server.cpp Server.h
	)

	fips_dir(sauce)
	fips_files(
		ServerModule.h
		ServerInjector.h
	)

	fips_deps(backend)
	copy_data_files(server)
fips_end_app()
check_lua_files(server behaviourtrees.lua)
check_lua_files(server attributes.lua)
