set(SRCS
	UUIDUtil.cpp UUIDUtil.h
)
set(LIB uuid)

if (WINDOWS)
	set(UUID_LIBRARIES rpcrt4)
elseif (APPLE)
	set(UUID_LIBRARIES "-framework CoreFoundation")
else()
	find_package(UUID)
endif()

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core ${UUID_LIBRARIES})

set(TEST_SRCS
	tests/UUIDTest.cpp
)

gtest_suite_files(tests ${TEST_FILES})
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
