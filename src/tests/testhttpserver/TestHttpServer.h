/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"
#include "http/HttpServer.h"
#include "console/TTY.h"

/**
 * @brief Test application to allow fuzzing the http server code
 *
 * See e.g. https://github.com/zardus/preeny and https://lolware.net/2015/04/28/nginx-fuzzing.html
 */
class TestHttpServer: public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;
	http::HttpServer _server;
	console::TTY _input;
	uv_loop_t *_loop = nullptr;
	core::VarPtr _exitAfterRequest;
	int _remainingFrames = 0;
public:
	TestHttpServer(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual app::AppState onConstruct() override;
	virtual app::AppState onInit() override;
	virtual app::AppState onRunning() override;
	virtual app::AppState onCleanup() override;
};
