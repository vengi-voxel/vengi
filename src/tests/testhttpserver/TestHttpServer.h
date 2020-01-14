/**
 * @file
 */

#pragma once

#include "core/ConsoleApp.h"
#include "http/HttpServer.h"
#include "core/Input.h"
#include <uv.h>

/**
 * @brief Test application to allow fuzzing the http server code
 *
 * See e.g. https://github.com/zardus/preeny and https://lolware.net/2015/04/28/nginx-fuzzing.html
 */
class TestHttpServer: public core::ConsoleApp {
private:
	using Super = core::ConsoleApp;
	http::HttpServer _server;
	core::Input _input;
	uv_loop_t *_loop = nullptr;
	core::VarPtr _exitAfterRequest;
	int _remainingFrames = 0;
public:
	TestHttpServer(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onRunning() override;
	virtual core::AppState onCleanup() override;
};
