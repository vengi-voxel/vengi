set(SRCS
	Connection.cpp Connection.h
	ScopedConnection.cpp ScopedConnection.h
	ConnectionPool.cpp ConnectionPool.h
	Model.cpp Model.h
	DBHandler.cpp DBHandler.h
	DBCondition.cpp DBCondition.h
	ConstraintType.h
	FieldType.h
	PreparedStatement.cpp PreparedStatement.h
	ScopedTransaction.cpp ScopedTransaction.h
	State.cpp State
	SQLGenerator.cpp SQLGenerator.h
	Timestamp.cpp Timestamp.h
)
set(LIB persistence)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

set(PostgreSQL_ADDITIONAL_VERSIONS "9.3" "9.4.1" "9.6")
set(PostgreSQL_INCLUDE_ADDITIONAL_SEARCH_SUFFIXES "postgresql96")
set(PostgreSQL_TYPE_ADDITIONAL_SEARCH_SUFFIXES "postgresql96/server")
set(PostgreSQL_LIBRARY_ADDITIONAL_SEARCH_SUFFIXES "postgresql96")
set(PostgreSQL_ADDITIONAL_SEARCH_PATHS "/usr/include/postgresql/9.3/server" "/usr/include/postgresql/9.4/server" "/opt/local/include/postgresql96/server")
find_package(PostgreSQL)
engine_target_link_libraries(TARGET persistence DEPENDENCIES ${PostgreSQL_LIBRARIES})
target_include_directories(persistence PUBLIC ${PostgreSQL_INCLUDE_DIRS})

if (POSTGRESQL_FOUND)
	gtest_suite_files(tests
		tests/ConnectionPoolTest.cpp
		tests/DBConditionTest.cpp
	)
	gtest_suite_deps(tests ${LIB})
endif()
