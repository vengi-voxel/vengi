project(aidebug)
set(SRCS
	network/AINetwork.cpp network/AINetwork.h
	network/MessageSender.h network/MessageSender.cpp

	AIDebug.h AIDebug.cpp
	Map.h Map.cpp
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES imgui ai-shared network attrib)

set(TEST_SRCS
	tests/AIMapTest.cpp
	Map.h Map.cpp
)

gtest_suite_sources(tests
	${TEST_SRCS}
)

gtest_suite_begin(tests-${PROJECT_NAME} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${PROJECT_NAME} ${TEST_SRCS})
gtest_suite_deps(tests-${PROJECT_NAME} test-app)
gtest_suite_end(tests-${PROJECT_NAME})
