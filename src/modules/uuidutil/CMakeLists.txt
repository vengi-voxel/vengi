set(LIBS core)

if (WINDOWS)
	list(APPEND LIBS rpcrt4)
elseif (APPLE)
	list(APPEND LIBS "-framework CoreFoundation")
else()
	find_package(UUID)
	if (UUID_LIBRARIES)
		list(APPEND LIBS ${UUID_LIBRARIES})
	endif()
endif()

set(SRCS
	UUIDUtil.cpp UUIDUtil.h
)

set(LIB uuidutil)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})

set(TEST_SRCS
	tests/UUIDTest.cpp
)

gtest_suite_files(tests ${TEST_FILES})
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
