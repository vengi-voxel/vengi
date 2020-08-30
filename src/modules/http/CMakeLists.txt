set(SRCS
	Http.h Http.cpp
	HttpClient.h HttpClient.cpp
	HttpHeader.h HttpHeader.cpp
	HttpMethod.h
	HttpMimeType.h
	HttpParser.h HttpParser.cpp
	HttpQuery.h
	HttpResponse.h
	HttpServer.h HttpServer.cpp
	HttpStatus.h HttpStatus.cpp
	Network.h Network.cpp.h Network.cpp
	ResponseParser.h ResponseParser.cpp
	RequestParser.h RequestParser.cpp
	Request.h Request.cpp
	Url.h Url.cpp
)
set(LIB http)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core)

set(TEST_SRCS
	tests/HttpClientTest.cpp
	tests/HttpHeaderTest.cpp
	tests/HttpServerTest.cpp
	tests/UrlTest.cpp
	tests/ResponseParserTest.cpp
	tests/RequestParserTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB} test-app)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
