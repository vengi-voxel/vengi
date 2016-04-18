fips_begin_module(persistence)
	fips_files(
		Connection.cpp Connection.h
		Store.cpp Store.h
		PeristenceModel.cpp PeristenceModel.h
	)
	fips_deps(core)
fips_end_module()

set(PostgreSQL_ADDITIONAL_VERSIONS "9.3" "9.4.1")
set(PostgreSQL_ADDITIONAL_SEARCH_PATHS "/usr/include/postgresql/9.3/server" "/usr/include/postgresql/9.4/server")
find_package(PostgreSQL REQUIRED)
target_link_libraries(persistence ${PostgreSQL_LIBRARIES})
target_include_directories(persistence PUBLIC ${PostgreSQL_INCLUDE_DIRS})

gtest_suite_files(tests tests/StoreTest.cpp)
gtest_suite_deps(tests persistence)
