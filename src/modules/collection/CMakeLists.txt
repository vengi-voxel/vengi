set(SRCS
	ConcurrentQueue.h
	ConcurrentSet.h
	Set.h
	dummy.cpp
)
set(LIB collection)
engine_add_module(TARGET ${LIB} SRCS ${SRCS})

set(TEST_SRCS
	tests/ConcurrentQueueTest.cpp
	tests/SetTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
