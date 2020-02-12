/**
 * @file
 */

#include "AbstractHttpParserTest.h"
#include "http/Http.h"
#include "http/HttpServer.h"

namespace http {

class ResponseParserTest : public AbstractHttpParserTest {
};

const char *ResponseBuf =
	"HTTP/1.0 200 OK\r\n"
	"Server: SimpleHTTP/0.6 Python/2.7.17\r\n"
	"Date: Tue, 10 Jan 2020 13:37:42 GMT\r\n"
	"Content-type: text/html; charset=UTF-8\r\n"
	"Content-Length: 1337\r\n"
	"\r\n"
	"<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
	"<html>\n"
	"</html>\n";

TEST_F(ResponseParserTest, testGETSimple) {
	ResponseParser response((uint8_t*)SDL_strdup(ResponseBuf), SDL_strlen(ResponseBuf));
	ASSERT_EQ(http::HttpStatus::Ok, response.status) << response.statusText;
	EXPECT_GE(response.headers.size(), 3u);
	validateMapEntry(response.headers, header::SERVER, "SimpleHTTP/0.6 Python/2.7.17");
	validateMapEntry(response.headers, header::CONTENT_TYPE, "text/html; charset=UTF-8");
	validateMapEntry(response.headers, header::CONTENT_LENGTH, "1337");
	EXPECT_EQ(71, response.contentLength);
	EXPECT_FALSE(response.valid()) << "Invalid content size should make this response invalid";
	const core::String r(response.content, response.contentLength);
	EXPECT_STREQ(r.c_str(), "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n<html>\n</html>\n");
}

TEST_F(ResponseParserTest, testCopy) {
	ResponseParser response(nullptr, 0u);
	{
		ResponseParser original((uint8_t*)SDL_strdup(ResponseBuf), SDL_strlen(ResponseBuf));
		response = original;
	}
	ASSERT_EQ(http::HttpStatus::Ok, response.status) << response.statusText;
	EXPECT_GE(response.headers.size(), 3u);
	validateMapEntry(response.headers, header::SERVER, "SimpleHTTP/0.6 Python/2.7.17");
	validateMapEntry(response.headers, header::CONTENT_TYPE, "text/html; charset=UTF-8");
	validateMapEntry(response.headers, header::CONTENT_LENGTH, "1337");
	EXPECT_EQ(71, response.contentLength);
	EXPECT_FALSE(response.valid()) << "Invalid content size should make this response invalid";
	const core::String r(response.content, response.contentLength);
	EXPECT_STREQ(r.c_str(), "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n<html>\n</html>\n");
}

TEST_F(ResponseParserTest, testGETChunk) {
	const char *ResponseBufChunk = "HTTP/1.1 200 OK\r\n"
		"Content-length: 8\r\n"
		"Server: server\r\n"
		"Content-Type: application/chunk\r\n"
		"Connection: close\r\n"
		"\r\n"
		"\a\a\a\a\a\a\a\a";
	ResponseParser response((uint8_t*)SDL_strdup(ResponseBufChunk), SDL_strlen(ResponseBufChunk));
	ASSERT_EQ(http::HttpStatus::Ok, response.status) << response.statusText;
	EXPECT_GE(response.headers.size(), 4u);
	validateMapEntry(response.headers, header::SERVER, "server");
	validateMapEntry(response.headers, header::CONTENT_TYPE, "application/chunk");
	validateMapEntry(response.headers, header::CONTENT_LENGTH, "8");
	EXPECT_EQ(8, response.contentLength);
	EXPECT_TRUE(response.valid()) << "Invalid content size should make this response invalid";
	EXPECT_EQ('\a', response.content[0]);
}

}
