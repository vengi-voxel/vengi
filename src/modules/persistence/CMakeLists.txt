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
	State.cpp State
	Structs.h
	Timestamp.cpp Timestamp.h
)

set(PostgreSQL_ADDITIONAL_VERSIONS "10")
find_package(PostgreSQL)
message(STATUS "Postgresql include dirs: ${PostgreSQL_INCLUDE_DIRS}")
message(STATUS "Postgresql libs: ${PostgreSQL_LIBRARIES}")

set(LIB persistence)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core ${PostgreSQL_LIBRARIES})

set(TEST_SRCS
	tests/DatabaseModelTest.cpp
	tests/SQLGeneratorTest.cpp
	tests/LongCounterTest.cpp
)

if (PostgreSQL_FOUND)
	target_include_directories(${LIB} PRIVATE ${PostgreSQL_INCLUDE_DIRS} /usr/include/postgresql/)
	set(HAVE_POSTGRES 1 CACHE INTERNAL "Found postgres")
	list(APPEND TEST_SRCS
		tests/PersistenceMgrTest.cpp
		tests/ConnectionPoolTest.cpp
		tests/DBConditionTest.cpp
		tests/DatabaseSchemaUpdateTest.cpp
	)
else()
	set(HAVE_POSTGRES 0 CACHE INTERNAL "Found postgres")
endif()

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

generate_db_models(${LIB} ${CMAKE_CURRENT_SOURCE_DIR}/tables.tbl PersistenceModels.h)
generate_db_models(tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/tests.tbl TestModels.h)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${LIB} ${LIB})
generate_db_models(tests-${LIB} ${CMAKE_CURRENT_SOURCE_DIR}/tests/tests.tbl TestModels.h)
gtest_suite_end(tests-${LIB})
