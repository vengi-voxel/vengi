set(SRCS
	HttpCacheStream.cpp HttpCacheStream.h
	Http.cpp Http.h
	Request.cpp Request.h
)

set(LIB http)
set(LIBS core io)
if (EMSCRIPTEN)
	list(APPEND SRCS system/Emscripten.h system/Emscripten.cpp)
elseif (UNIX)
	find_package(CURL)

	if(CURL_FOUND)
		list(APPEND LIBS CURL::libcurl)
		list(APPEND SRCS system/Curl.h system/Curl.cpp)
		message(STATUS "Found libcurl")
	else()
		list(APPEND SRCS system/Null.h system/Null.cpp)
		message(WARNING "No libcurl found")
	endif()

	set(USE_CURL ${CURL_FOUND} CACHE BOOL "Use curl for https requests" FORCE)
elseif (WINDOWS)
	list(APPEND SRCS system/WinHttp.h system/WinHttp.cpp)
endif()

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})

set(TEST_SRCS
	tests/RequestTest.cpp
	tests/HttpCacheStreamTest.cpp
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} test-app json)
gtest_suite_end(tests-${LIB})
