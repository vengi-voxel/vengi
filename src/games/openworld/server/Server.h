/**
 * @file
 */

#pragma once

#include "console/TextConsoleApp.h"
#include "backend/loop/ServerLoop.h"
#include "core/TimeProvider.h"
#include "http/HttpServer.h"

class Server: public console::TextConsoleApp {
private:
	using Super = console::TextConsoleApp;
	backend::ServerLoopPtr _serverLoop;
public:
	Server(const metric::MetricPtr& metric, const backend::ServerLoopPtr& serverLoop,
			const core::TimeProviderPtr& timeProvider, const io::FilesystemPtr& filesystem,
			const core::EventBusPtr& eventBus, const http::HttpServerPtr& httpServer);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;
	app::AppState onRunning() override;
};
