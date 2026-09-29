/**
 * @file
 */

#include "http/RequestAsync.h"
#include "http/Http.h"
#include "app/tests/AbstractTest.h"
#include "core/concurrent/Atomic.h"
#include <SDL3/SDL_thread.h>
#include <gtest/gtest.h>

namespace http {

class RequestAsyncTest : public app::AbstractTest {};

TEST_F(RequestAsyncTest, testCallbackRunsOnMainThread) {
	if (!Request::supported()) {
		GTEST_SKIP() << "No http support available";
	}

	core::AtomicBool called{false};
	core::AtomicBool onMainThread{false};
	core::AtomicInt statusCode{-1};

	// Connection refused / fast failure is enough to exercise the async path.
	Request request("http://127.0.0.1:1/", RequestType::GET);
	request.setTimeoutSecond(1);
	request.setConnectTimeoutSecond(1);
	ASSERT_NE(0u, requestAsync(core::move(request), [&](const Response &response) {
		called = true;
		onMainThread = SDL_IsMainThread();
		statusCode = response.statusCode;
		EXPECT_FALSE(response.url.empty());
	}));

	bool finished = false;
	for (int i = 0; i < 200 && !finished; ++i) {
		http::update();
		if (called) {
			finished = true;
			break;
		}
		_testApp->wait(10);
	}

	ASSERT_TRUE(finished) << "async http callback was not dispatched";
	EXPECT_TRUE(onMainThread);
	clearPending();
}

TEST_F(RequestAsyncTest, testClearPendingDropsCallback) {
	if (!Request::supported()) {
		GTEST_SKIP() << "No http support available";
	}

	core::AtomicBool called{false};
	Request request("http://127.0.0.1:1/", RequestType::GET);
	request.setTimeoutSecond(1);
	request.setConnectTimeoutSecond(1);
	ASSERT_NE(0u, requestAsync(core::move(request), [&](const Response &) { called = true; }));
	clearPending();

	for (int i = 0; i < 50; ++i) {
		http::update();
		_testApp->wait(10);
	}
	EXPECT_FALSE(called);
}

} // namespace http
