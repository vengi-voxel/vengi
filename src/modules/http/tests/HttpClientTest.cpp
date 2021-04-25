/**
 * @file
 */

#include "core/concurrent/Atomic.h"
#include "app/tests/AbstractTest.h"
#include "http/HttpClient.h"
#include "http/HttpServer.h"
#include "core/concurrent/ThreadPool.h"
#include "core/concurrent/Lock.h"
#include "core/concurrent/ConditionVariable.h"

namespace http {

class HttpClientTest : public app::AbstractTest {
};

TEST_F(HttpClientTest, testSimple) {
	core_trace_mutex(core::Lock, serverStartMutex, "Server start mutex");
	core::ConditionVariable startCondition;
	core::AtomicBool serverSuccess { false };
	core::AtomicBool finishedSetup { false };

	core::ScopedLock lock(serverStartMutex);
	_testApp->threadPool().enqueue([this, &startCondition, &serverSuccess, &finishedSetup] () {
		http::HttpServer _httpServer(_testApp->metric());
		serverSuccess = _httpServer.init(8095);
		if (!serverSuccess) {
			Log::error("Failed to initialize the http server on port 8095");
			finishedSetup = true;
			startCondition.notify_one();
			return;
		}
		_httpServer.registerRoute(http::HttpMethod::GET, "/", [] (const http::RequestParser& request, HttpResponse* response) {
			response->setText("Success");
		});
		finishedSetup = true;
		startCondition.notify_one();

		while (_testApp->state() == app::AppState::Running) {
			_httpServer.update();
		}
		_httpServer.shutdown();
	});
	startCondition.wait(serverStartMutex, [&finishedSetup] {
		return (bool)finishedSetup;
	});
	if (!serverSuccess) {
		return;
	}
	HttpClient client("http://localhost:8095");
	client.setRequestTimeout(1);
	ResponseParser response = client.get("/");
	ASSERT_TRUE(response.valid()) << "Invalid response";
	const char *length = "";
	EXPECT_TRUE(response.headers.get(http::header::CONTENT_LENGTH, length));
	EXPECT_STREQ("7", length);
	const char *type = "";
	EXPECT_TRUE(response.headers.get(http::header::CONTENT_TYPE, type));
	EXPECT_STREQ("text/plain", type);
}

}
