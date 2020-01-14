/**
 * @file
 */

#include "TestHttpServer.h"
#include "core/io/Filesystem.h"
#include "core/metric/Metric.h"
#include "core/EventBus.h"
#include "core/TimeProvider.h"
#include "core/Var.h"

TestHttpServer::TestHttpServer(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testhttpserver");
}

core::AppState TestHttpServer::onConstruct() {
	core::AppState state = Super::onConstruct();
	_framesPerSecondsCap->setVal(5.0f);
	_exitAfterRequest = core::Var::get("exitafterrequest", "0");
	return state;
}

core::AppState TestHttpServer::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	_loop = new uv_loop_t;
	if (uv_loop_init(_loop) != 0) {
		Log::error("Failed to init event loop");
		uv_loop_close(_loop);
		delete _loop;
		_loop = nullptr;
		return core::AppState::InitFailure;
	}

	if (!_input.init(_loop)) {
		Log::warn("Could not init console input");
	}

	const int16_t port = 8088;
	if (!_server.init(port)) {
		Log::error("Failed to start the http server");
		return core::AppState::InitFailure;
	}

	_server.registerRoute(http::HttpMethod::GET, "/", [&] (const http::RequestParser& request, http::HttpResponse* response) {
		Log::error("Got a request for /");
		if (_exitAfterRequest->intVal() > 0) {
			_remainingFrames = _exitAfterRequest->intVal();
			response->setText("Request successful - shutting down the server\n");
		} else {
			response->setText("Request successful\n");
		}
	});

	_server.setErrorText(http::HttpStatus::NotFound, "Not found\n");

	_server.registerRoute(http::HttpMethod::GET, "/shutdown", [&] (const http::RequestParser& requesty, http::HttpResponse* response) {
		Log::error("Got a shutdown request");
		response->setText("Request successful - shutting down the server after 5 steps\n");
		_remainingFrames = 5;
	});

	Log::info("Running on port %i with %.1f fps", (int)port, _framesPerSecondsCap->floatVal());

	Log::info("Use cvar '%s' to shut down after a request", _exitAfterRequest->name().c_str());
	return state;
}

core::AppState TestHttpServer::onRunning() {
	Super::onRunning();
	uv_run(_loop, UV_RUN_NOWAIT);
	_server.update();
	if (_remainingFrames > 0) {
		if (--_remainingFrames <= 0) {
			requestQuit();
		} else {
			Log::info("%i steps until shutdown", _remainingFrames);
		}
	}
	return core::AppState::Running;
}

core::AppState TestHttpServer::onCleanup() {
	core::AppState state = Super::onCleanup();
	if (_loop != nullptr) {
		uv_tty_reset_mode();
		uv_loop_close(_loop);
		delete _loop;
		_loop = nullptr;
	}
	Log::info("Shuttting down http server");
	_server.shutdown();
	return state;
}

CONSOLE_APP(TestHttpServer)
