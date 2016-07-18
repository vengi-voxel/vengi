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

find_package(PostgreSQL)
if (PostgreSQL_FOUND)
	target_link_libraries(persistence ${PostgreSQL_LIBRARIES})
	target_include_directories(persistence PUBLIC ${PostgreSQL_INCLUDE_DIRS})
else()
	target_link_libraries(${LIB} sqlite3)
endif()

gtest_suite_files(tests
	tests/ConnectionPoolTest.cpp
)
gtest_suite_deps(tests ${LIB})
