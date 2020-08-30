/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "http/HttpServer.h"

namespace http {

class HttpServerTest : public app::AbstractTest {
};

TEST_F(HttpServerTest, testSimple) {
	HttpServer server(_testApp->metric());
	EXPECT_TRUE(server.init(10101));
	server.registerRoute(HttpMethod::GET, "/", [] (const http::RequestParser& request, HttpResponse* response) {
	});
	EXPECT_TRUE(server.unregisterRoute(HttpMethod::GET, "/"));
	server.shutdown();
}

}
