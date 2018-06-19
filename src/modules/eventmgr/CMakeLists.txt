set(SRCS
	EventMgr.h EventMgr.cpp
	Event.h Event.cpp
	EventProvider.h EventProvider.cpp
	EventConfigurationData.h
	EventId.h
	EventType.h
	LUAFunctions.h
)
set(LIB eventmgr)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core commonlua network persistence)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
generate_db_models(${LIB} ${CMAKE_CURRENT_SOURCE_DIR}/tables.tbl EventMgrModels.h)

gtest_suite_files(tests
	tests/EventMgrTest.cpp
)
gtest_suite_files(tests
	tests/test-events.lua
)
gtest_suite_deps(tests ${LIB})
