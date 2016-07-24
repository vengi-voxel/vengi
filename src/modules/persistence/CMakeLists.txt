set(SRCS
	Connection.cpp Connection.h
	ScopedConnection.cpp ScopedConnection.h
	ConnectionPool.cpp ConnectionPool.h
	Model.cpp Model.h
	Timestamp.h
)
set(LIB persistence)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

set(PostgreSQL_ADDITIONAL_VERSIONS "9.3" "9.4.1")
set(PostgreSQL_ADDITIONAL_SEARCH_PATHS "/usr/include/postgresql/9.3/server" "/usr/include/postgresql/9.4/server")
find_package(PostgreSQL REQUIRED)
target_link_libraries(persistence ${PostgreSQL_LIBRARIES})
target_include_directories(persistence PUBLIC ${PostgreSQL_INCLUDE_DIRS})

gtest_suite_files(tests
	tests/ConnectionPoolTest.cpp
)
gtest_suite_deps(tests ${LIB})
