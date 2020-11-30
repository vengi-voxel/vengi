set(SRCS
	AttribUpdateHandler.h
	AuthFailedHandler.h
	ClientMessageSender.cpp ClientMessageSender.h
	ClientNetwork.cpp ClientNetwork.h
	EntityRemoveHandler.h
	EntitySpawnHandler.h
	EntityUpdateHandler.h
	IClientProtocolHandler.h
	SignupValidationStateHandler.h
	StartCooldownHandler.h
	StopCooldownHandler.h
	UserInfoHandler.h
	UserSpawnHandler.h
	VarUpdateHandler.h
)
set(LIB client-network)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES shared)
target_include_directories(${LIB} PUBLIC ..)

set(TEST_SRCS
	tests/ConnectTest.cpp
)
gtest_suite_files(tests ${TEST_FILES})
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app backend)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app backend)
gtest_suite_end(tests-${LIB})
