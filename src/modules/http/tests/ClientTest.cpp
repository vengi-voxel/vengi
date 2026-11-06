/**
 * @file
 */

#include "http/Client.h"
#include "app/tests/AbstractTest.h"
#include "core/StringUtil.h"
#include "core/concurrent/Thread.h"
#include "engine-config.h"
#include "io/File.h"
#include <SDL3/SDL_stdinc.h>

namespace http {

class ClientTest : public app::AbstractTest {
protected:
	bool dispatchUntil(const bool &done) {
		for (int i = 0; i < 2000 && !done; ++i) {
			http::update();
			wait(5);
		}
		return done;
	}

	Request failingRequest() {
		Request request("http://127.0.0.1:1/", RequestType::GET);
		request.setTimeoutSecond(1);
		request.setConnectTimeoutSecond(1);
		return request;
	}
};

TEST_F(ClientTest, testDeferredFailureOnUpdatingThread) {
	if (!Request::supported()) {
		GTEST_SKIP();
	}
	Client client;
	bool called = false;
	const core::ThreadId thread = core::getCurrentThreadId();
	// Keep ASSERT_* outside the callback: nesting gtest macros in a lambda that is
	// itself an ASSERT_* argument triggers -Wshadow on gtest_ar.
	const RequestId id = client.execute(failingRequest(), [&](const Response &response) {
		EXPECT_EQ(thread, core::getCurrentThreadId());
		EXPECT_FALSE(response.success);
		EXPECT_EQ(0, response.statusCode);
		EXPECT_EQ("http://127.0.0.1:1/", response.url);
		called = true;
	});
	ASSERT_NE(0u, id);
	EXPECT_FALSE(called);
	ASSERT_TRUE(dispatchUntil(called));
}

TEST_F(ClientTest, testCancellationAndOwnerDestruction) {
	if (!Request::supported()) {
		GTEST_SKIP();
	}
	Client live;
	bool cancelledCalled = false;
	bool destroyedCalled = false;
	bool called = false;
	const RequestId id = live.execute(failingRequest(), [&](const Response &) { cancelledCalled = true; });
	ASSERT_NE(0u, id);
	EXPECT_TRUE(live.cancel(id));
	EXPECT_FALSE(live.cancel(id));
	{
		Client temporary;
		ASSERT_NE(0u, temporary.execute(failingRequest(), [&](const Response &) { destroyedCalled = true; }));
		http::update();
		wait(20);
	}
	ASSERT_NE(0u, live.execute(failingRequest(), [&](const Response &) { called = true; }));
	ASSERT_TRUE(dispatchUntil(called));
	EXPECT_FALSE(cancelledCalled);
	EXPECT_FALSE(destroyedCalled);
}

TEST_F(ClientTest, testCallbackCanCancelAndSubmit) {
	if (!Request::supported()) {
		GTEST_SKIP();
	}
	Client client;
	bool cancelledCalled = false;
	bool called = false;
	RequestId other = 0;
	ASSERT_NE(0u, client.execute(failingRequest(), [&](const Response &) {
		client.cancel(other);
		client.execute(failingRequest(), [&](const Response &) { called = true; });
		http::update();
	}));
	other = client.execute(failingRequest(), [&](const Response &) { cancelledCalled = true; });
	ASSERT_NE(0u, other);
	// Allow both responses to complete before dispatching either callback.
	http::update();
	wait(2500);
	ASSERT_TRUE(dispatchUntil(called));
	EXPECT_FALSE(cancelledCalled);
}

TEST_F(ClientTest, testShutdownAndRestart) {
	Client client;
	bool called = false;
	client.execute(failingRequest(), [&](const Response &) { called = true; });
	http::update();
	http::shutdown();
	http::update();
	EXPECT_FALSE(called);
	EXPECT_EQ(0u, client.execute(failingRequest()));
	http::init();
	if (Request::supported()) {
		ASSERT_NE(0u, client.execute(failingRequest(), [&](const Response &) { called = true; }));
		ASSERT_TRUE(dispatchUntil(called));
	}
}

TEST_F(ClientTest, testSubmissionFromWorker) {
	if (!Request::supported()) {
		GTEST_SKIP();
	}
	Client client;
	const Request request = failingRequest();
	bool called = false;
	RequestId id = 0;
	core::Thread thread([&]() { id = client.execute(request, [&](const Response &) { called = true; }); },
						"http-submit-test");
	thread.join();
	ASSERT_NE(0u, id);
	EXPECT_FALSE(called);
	EXPECT_TRUE(dispatchUntil(called));
}

TEST_F(ClientTest, testHttpResponse) {
	const char *url = SDL_getenv("VENGI_HTTP_TEST_URL");
	if (!url || !Request::supported()) {
		GTEST_SKIP() << "Start tests/server.py and set VENGI_HTTP_TEST_URL to its address";
	}
	Client client;
	bool called = false;
	const char body[] = {'a', '\0', 'b', '\xff'};
	{
		Request request(url, RequestType::POST);
		request.setBody(body, sizeof(body));
		request.addHeader("X-Test", "async-http");
		const RequestId id = client.execute(request, [&](const Response &response) {
			EXPECT_TRUE(response.success);
			EXPECT_EQ(404, response.statusCode);
			core::String header;
			EXPECT_TRUE(response.headers.get("X-Test", header));
			EXPECT_EQ("async-http", header);
			ASSERT_EQ(sizeof(body), response.body.size());
			EXPECT_EQ(0, memcmp(body, response.body.data(), sizeof(body)));
			const Response copy = response;
			EXPECT_EQ(sizeof(body), copy.body.size());
			EXPECT_EQ(0, memcmp(body, copy.body.data(), sizeof(body)));
			called = true;
		});
		ASSERT_NE(0u, id);
		request.setBody("changed after submission");
	}
	ASSERT_TRUE(dispatchUntil(called));
}

TEST_F(ClientTest, testQueueLimitAndCancelAll) {
	if (!Request::supported()) {
		GTEST_SKIP();
	}
	Client client;
	const Request request = failingRequest();
	for (int i = 0; i < 64; ++i) {
		ASSERT_NE(0u, client.execute(request));
	}
	EXPECT_EQ(0u, client.execute(request));
	client.cancelAll();
	http::update();
	EXPECT_NE(0u, client.execute(request));
}

// A local file exercises byte ownership without requiring an external service.
TEST_F(ClientTest, testBinaryResponse) {
#if USE_CURL
	const core::String path = _testApp->filesystem()->homeWritePath("http-binary-response");
	const char data[] = {'a', '\0', 'b', '\xff'};
	ASSERT_TRUE(io::Filesystem::sysWrite(path, (const uint8_t *)data, sizeof(data)));
	Client client;
	bool called = false;
	{
		Request request("file://" + core::string::urlPathEncode(path), RequestType::GET);
		const RequestId id = client.execute(request, [&](const Response &response) {
			EXPECT_TRUE(response.success);
			ASSERT_EQ(sizeof(data), response.body.size());
			EXPECT_EQ(0, memcmp(data, response.body.data(), sizeof(data)));
			called = true;
		});
		ASSERT_NE(0u, id);
	}
	ASSERT_TRUE(dispatchUntil(called));
	io::Filesystem::sysRemoveFile(path);
#else
	GTEST_SKIP();
#endif
}

} // namespace http
