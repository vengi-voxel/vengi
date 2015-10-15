fips_begin_module(dbpost)
	fips_files(
		PQConnect.cpp PQConnect.h
		PQStore.cpp PQStore.h
		StoreInterface.cpp StoreInterface.h
	)
	set(PostgreSQL_ADDITIONAL_VERSIONS "9.3" "9.4.1")
	set(PostgreSQL_ADDITIONAL_SEARCH_PATHS "/usr/include/postgresql/9.3/server" "/usr/include/postgresql/9.4/server")
	find_package(PostgreSQL REQUIRED)
	include_directories(${PostgreSQL_INCLUDE_DIRS})
	fips_deps(core ${PostgreSQL_LIBRARIES})
fips_end_module()

begin_googleunittest(dbpost)
	fips_dir(tests)
	fips_files(
		PQStoreTest.cpp
	)
	fips_deps(dbpost)
end_googleunittest()
