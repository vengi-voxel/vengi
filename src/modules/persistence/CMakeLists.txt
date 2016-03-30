fips_begin_module(persistence)
	fips_files(
		Connection.cpp Connection.h
		Store.cpp Store.h
		PeristenceModel.cpp PeristenceModel.h
	)
	set(PostgreSQL_ADDITIONAL_VERSIONS "9.3" "9.4.1")
	set(PostgreSQL_ADDITIONAL_SEARCH_PATHS "/usr/include/postgresql/9.3/server" "/usr/include/postgresql/9.4/server")
	find_package(PostgreSQL REQUIRED)
	include_directories(${PostgreSQL_INCLUDE_DIRS})
	fips_deps(core ${PostgreSQL_LIBRARIES})
fips_end_module()

gtest_suite_files(tests tests/StoreTest.cpp)
gtest_suite_deps(tests persistence)
