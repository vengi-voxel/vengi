fips_begin_app(server cmdline)
	fips_files(
		Server.cpp Server.h
	)

	fips_dir(sauce)
	fips_files(
		ServerModule.h
		ServerInjector.h
	)

	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS} .)
	if (FIPS_LINUX)
		list(APPEND DEPS "-latomic")
	endif()
	fips_deps(backend ${DEPS} ${SDL2_LIBRARIES})
	copy_data_files(server)
fips_end_app()
check_lua_files(server behaviourtrees.lua)
check_lua_files(server attributes.lua)
