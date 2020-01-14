/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "http/HttpClient.h"
#include "http/HttpServer.h"

namespace http {

class HttpClientTest : public core::AbstractTest {
};

TEST_F(HttpClientTest, testSimple) {
	_testApp->threadPool().enqueue([this] () {
		http::HttpServer _httpServer;
		_httpServer.init(8095);
		_httpServer.registerRoute(http::HttpMethod::GET, "/", [] (const http::RequestParser& request, HttpResponse* response) {
			response->setText("Success");
		});
		while (_testApp->state() == core::AppState::Running) {
			_httpServer.update();
		}
		_httpServer.shutdown();
	});
	HttpClient client("http://localhost:8095");
	ResponseParser response = client.get("/");
	EXPECT_TRUE(response.valid());
	const char *length;
	EXPECT_TRUE(response.headers.get(http::header::CONTENT_LENGTH, length));
	EXPECT_STREQ("7", length);
	const char *type;
	EXPECT_TRUE(response.headers.get(http::header::CONTENT_LENGTH, type));
	EXPECT_STREQ("text/plain", length);
}

}
