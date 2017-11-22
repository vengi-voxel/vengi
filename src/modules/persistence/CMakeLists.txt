set(SRCS
	BindParam.cpp BindParam.h
	Connection.cpp Connection.h
	ConnectionPool.cpp ConnectionPool.h
	ConstraintType.h
	DBCondition.cpp DBCondition.h
	DBHandler.cpp DBHandler.h
	Field.h
	FieldType.h
	ForwardDecl.h
	Model.cpp Model.h
	Order.h
	OrderBy.h
	ScopedConnection.cpp ScopedConnection.h
	ScopedTransaction.cpp ScopedTransaction.h
	SQLGenerator.cpp SQLGenerator.h
	State.cpp State
	Structs.h
	Timestamp.cpp Timestamp.h
)
set(LIB persistence)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

set(PostgreSQL_ADDITIONAL_VERSIONS "10")
find_package(PostgreSQL)
message(STATUS "Postgresql include dirs: ${PostgreSQL_INCLUDE_DIRS}")
message(STATUS "Postgresql libs: ${PostgreSQL_LIBRARIES}")
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES ${PostgreSQL_LIBRARIES})
target_include_directories(${LIB} PUBLIC ${PostgreSQL_INCLUDE_DIRS} /usr/include/postgresql/)

set(TEST_SRCS
	tests/DatabaseModelTest.cpp
)

if (POSTGRESQL_FOUND)
	set(HAVE_POSTGRES 1 CACHE INTERNAL "Found postgres")
	list(APPEND TEST_SRCS
		tests/ConnectionPoolTest.cpp
		tests/DBConditionTest.cpp
	)
else()
	set(HAVE_POSTGRES 0 CACHE INTERNAL "Found postgres")
endif()

gtest_suite_files(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

generate_db_models(${LIB} ${CMAKE_CURRENT_SOURCE_DIR}/tables.tbl PersistenceModels.h)
test_generate_db_models(tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/tests.tbl TestModels.h)
