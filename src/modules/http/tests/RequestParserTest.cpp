/**
 * @file
 */

#include "AbstractHttpParserTest.h"
#include "http/RequestParser.h"

namespace http {

class RequestParserTest : public AbstractHttpParserTest {
protected:
	void testQueryParams(const char *queryLine, const char *expected) {
		core::String hdr = core::String::format(
			"GET %s HTTP/1.1\r\n"
			"Host: localhost:8088\r\n"
			"User-Agent: curl/7.67.0\r\n"
			"Accept: */*\r\n"
			"\r\n", queryLine);
		RequestParser request((uint8_t*)SDL_strdup(hdr.c_str()), hdr.size());
		EXPECT_TRUE(request.valid());
		EXPECT_STREQ("HTTP/1.1", request.protocolVersion);
		EXPECT_EQ(HttpMethod::GET, request.method);
		EXPECT_STREQ(expected, request.path);
	}

	void testQueryParamsValid(const char *queryLine) {
		testQueryParams(queryLine, queryLine);
	}
};

TEST_F(RequestParserTest, testSimple) {
	char *buf = SDL_strdup(
		"GET / HTTP/1.1\r\n"
		"Host: localhost:8088\r\n"
		"User-Agent: curl/7.67.0\r\n"
		"Accept: */*\r\n"
		"\r\n");
	RequestParser request((uint8_t*)buf, SDL_strlen(buf));
	EXPECT_TRUE(request.valid());
	EXPECT_STREQ("HTTP/1.1", request.protocolVersion);
	EXPECT_EQ(HttpMethod::GET, request.method);
	EXPECT_STREQ("/", request.path);
	EXPECT_EQ(request.headers.size(), 3u);
	validateMapEntry(request.headers, header::HOST, "localhost:8088");
}

TEST_F(RequestParserTest, testCopy) {
	char *buf = SDL_strdup(
		"GET / HTTP/1.1\r\n"
		"Host: localhost:8088\r\n"
		"User-Agent: curl/7.67.0\r\n"
		"Accept: */*\r\n"
		"\r\n");
	RequestParser request(nullptr, 0u);
	{
		RequestParser original((uint8_t*)buf, SDL_strlen(buf));
		request = original;
	}
	EXPECT_TRUE(request.valid());
	EXPECT_STREQ("HTTP/1.1", request.protocolVersion);
	EXPECT_EQ(HttpMethod::GET, request.method);
	EXPECT_STREQ("/", request.path);
	EXPECT_EQ(request.headers.size(), 3u);
	validateMapEntry(request.headers, header::HOST, "localhost:8088");
}

TEST_F(RequestParserTest, testQuery) {
	char *buf = SDL_strdup(
		"GET /foo?param=value&param2=value&param3&param4=1 HTTP/1.1\r\n"
		"Host: localhost:8088\r\n"
		"User-Agent: curl/7.67.0\r\n"
		"Accept: */*\r\n"
		"\r\n");
	RequestParser request((uint8_t*)buf, SDL_strlen(buf));
	EXPECT_TRUE(request.valid());
	EXPECT_STREQ("HTTP/1.1", request.protocolVersion);
	EXPECT_EQ(HttpMethod::GET, request.method);
	EXPECT_STREQ("/foo", request.path);
	EXPECT_EQ(request.query.size(), 4u);
	validateMapEntry(request.query, "param", "value");
	validateMapEntry(request.query, "param2", "value");
	validateMapEntry(request.query, "param3", "");
	validateMapEntry(request.query, "param4", "1");
}

TEST_F(RequestParserTest, testQueryInvalidFirstParamSep) {
	testQueryParams("/foo&param=value", "/foo&param=value");
}

TEST_F(RequestParserTest, testQueryEmptyParams) {
	testQueryParams("/foo?", "/foo");
}

TEST_F(RequestParserTest, testQueryEmptyParamsInvalid) {
	testQueryParams("/foo?&", "/foo");
}

TEST_F(RequestParserTest, testQueryNoPathEmptyParams) {
	testQueryParams("?", "");
}

TEST_F(RequestParserTest, testQueryNoPathButParams) {
	testQueryParams("?foo=1", "");
}

}
