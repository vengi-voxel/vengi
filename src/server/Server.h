/**
 * @file
 */

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
public:
	Server(const network::NetworkPtr& network, const backend::ServerLoopPtr& serverLoop,
			const core::TimeProviderPtr& timeProvider, const io::FilesystemPtr& filesystem,
			const core::EventBusPtr& eventBus);

	core::AppState onInit() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
};
