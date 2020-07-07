/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "http/HttpClient.h"
#include "http/HttpServer.h"
#include "core/concurrent/ThreadPool.h"
#include "core/concurrent/Lock.h"
#include "core/concurrent/ConditionVariable.h"

namespace http {

class HttpClientTest : public core::AbstractTest {
};

TEST_F(HttpClientTest, testSimple) {
	core_trace_mutex(core::Lock, serverStartMutex, "Server start mutex");
	core::ConditionVariable startCondition;

	core::ScopedLock lock(serverStartMutex);
	_testApp->threadPool().enqueue([this, &startCondition] () {
		http::HttpServer _httpServer(_testApp->metric());
		_httpServer.init(8095);
		_httpServer.registerRoute(http::HttpMethod::GET, "/", [] (const http::RequestParser& request, HttpResponse* response) {
			response->setText("Success");
		});
		startCondition.notify_one();

		while (_testApp->state() == core::AppState::Running) {
			_httpServer.update();
		}
		_httpServer.shutdown();
	});
	startCondition.wait(serverStartMutex, [] {
		return true;
	});
	HttpClient client("http://localhost:8095");
	ResponseParser response = client.get("/");
	EXPECT_TRUE(response.valid());
	const char *length = "";
	EXPECT_TRUE(response.headers.get(http::header::CONTENT_LENGTH, length));
	EXPECT_STREQ("7", length);
	const char *type;
	EXPECT_TRUE(response.headers.get(http::header::CONTENT_TYPE, type));
	EXPECT_STREQ("text/plain", type);
}

}
