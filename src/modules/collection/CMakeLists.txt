set(SRCS
	ConcurrentQueue.h
	ConcurrentSet.h
	Set.h
	dummy.cpp
)
set(LIB collection)
add_library(${LIB} ${SRCS})
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

set(TEST_SRCS
	tests/ConcurrentQueueTest.cpp
	tests/SetTest.cpp
)

gtest_suite_files(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})
