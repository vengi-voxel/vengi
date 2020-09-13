set(SRCS
	EventMgr.h EventMgr.cpp
	Event.h Event.cpp
	EventProvider.h EventProvider.cpp
	EventConfigurationData.h
	EventId.h
	EventType.h
	LUAEventMgr.h LUAEventMgr.cpp
)
set(LIB eventmgr)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES commonlua network persistence)
generate_db_models(${LIB} ${CMAKE_CURRENT_SOURCE_DIR}/tables.tbl EventMgrModels.h)

set(TEST_SRCS
	tests/EventMgrTest.cpp
)
set(TEST_FILES
	tests/test-events.lua
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_files(tests ${TEST_FILES})
gtest_suite_deps(tests ${LIB} test-app)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
