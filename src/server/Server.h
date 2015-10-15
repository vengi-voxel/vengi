#pragma once

#include "core/App.h"
#include "backend/loop/ServerLoop.h"
#include "core/TimeProvider.h"
#include "network/Network.h"

class Server: public core::App {
private:
	bool _quit;
	network::NetworkPtr _network;
	backend::ServerLoopPtr _serverLoop;
	core::TimeProviderPtr _timeProvider;
public:
	Server(network::NetworkPtr network, backend::ServerLoopPtr serverLoop, core::TimeProviderPtr timeProvider, io::FilesystemPtr filesystem, core::EventBusPtr eventBus);

	core::AppState onInit() override;

	core::AppState onRunning() override;
};
