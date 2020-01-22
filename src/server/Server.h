/**
 * @file
 */

#pragma once

#include "console/CursesApp.h"
#include "backend/loop/ServerLoop.h"
#include "core/TimeProvider.h"
#include "http/HttpServer.h"

class Server: public console::CursesApp {
private:
	using Super = console::CursesApp;
	backend::ServerLoopPtr _serverLoop;
public:
	Server(const metric::MetricPtr& metric, const backend::ServerLoopPtr& serverLoop,
			const core::TimeProviderPtr& timeProvider, const io::FilesystemPtr& filesystem,
			const core::EventBusPtr& eventBus, const http::HttpServerPtr& httpServer);

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
};
