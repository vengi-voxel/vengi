/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "http/HttpServer.h"

namespace http {

class HttpServerTest : public core::AbstractTest {
};

TEST_F(HttpServerTest, testSimple) {
	HttpServer server;
	EXPECT_TRUE(server.init(10101));
	server.registerRoute(HttpMethod::GET, "/", [] (const http::RequestParser& request, HttpResponse* response) {
	});
	EXPECT_TRUE(server.unregisterRoute(HttpMethod::GET, "/"));
	server.shutdown();
}

}
