set(SRCS
	BindParam.cpp BindParam.h
	Connection.cpp Connection.h
	ConnectionPool.cpp ConnectionPool.h
	ConstraintType.h
	DBCondition.cpp DBCondition.h
	DBHandler.cpp DBHandler.h
	Field.h
	FieldType.cpp FieldType.h
	ForwardDecl.h
	Model.cpp Model.h
	Order.h
	OrderBy.h
	ISavable.h
	LongCounter.h
	MassQuery.cpp MassQuery.h
	PersistenceMgr.cpp PersistenceMgr.h
	ScopedConnection.cpp ScopedConnection.h
	ScopedTransaction.cpp ScopedTransaction.h
	SQLGenerator.cpp SQLGenerator.h
	State.cpp State.h
	Structs.h
	Timestamp.cpp Timestamp.h

	postgres/PQSymbol.h postgres/PQSymbol.cpp
)

set(PostgreSQL_ADDITIONAL_VERSIONS "10;10.4;11")
find_package(PostgreSQL)

set(LIB persistence)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core)

set(TEST_SRCS
	tests/DatabaseModelTest.cpp
	tests/SQLGeneratorTest.cpp
	tests/LongCounterTest.cpp
	tests/Mocks.h
)

if (PostgreSQL_FOUND)
	target_include_directories(${LIB} PRIVATE ${PostgreSQL_INCLUDE_DIRS} /usr/include/postgresql/)
	list(APPEND TEST_SRCS
		tests/PersistenceMgrTest.cpp
		tests/ConnectionPoolTest.cpp
		tests/DBConditionTest.cpp
		tests/DatabaseSchemaUpdateTest.cpp
	)
else()
	message(WARNING "No postgres was found on your system. Make sure to have libpq and pg_type.h installed on your system")
endif()
set(HAVE_POSTGRES ${PostgreSQL_FOUND} CACHE BOOL "Found postgres" FORCE)

message(STATUS "Postgresql include dirs: ${PostgreSQL_INCLUDE_DIRS}")
message(STATUS "Postgresql libs: ${PostgreSQL_LIBRARIES}")
message(STATUS "Postgresql found: ${HAVE_POSTGRES}")

generate_db_models(${LIB} ${CMAKE_CURRENT_SOURCE_DIR}/tables.tbl PersistenceModels.h)

if (TESTS)
	gtest_suite_sources(tests ${TEST_SRCS})
	gtest_suite_deps(tests ${LIB})

	generate_db_models(tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/tests.tbl TestModels.h)

	gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
	gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
	gtest_suite_deps(tests-${LIB} ${LIB})
	generate_db_models(tests-${LIB} ${CMAKE_CURRENT_SOURCE_DIR}/tests/tests.tbl TestModels.h)
	gtest_suite_end(tests-${LIB})
endif()
