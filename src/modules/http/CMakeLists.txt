set(SRCS
	Request.cpp Request.h
)

set(LIB http)
set(LIBS core io)
find_package(CURL)

if(CURL_FOUND)
	list(APPEND LIBS CURL::libcurl)
endif()

set(USE_CURL ${CURL_FOUND} CACHE BOOL "Use curl for https requests" FORCE)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})

set(TEST_SRCS
	tests/RequestTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
