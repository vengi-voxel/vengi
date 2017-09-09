/**
 * @file
 */

#pragma once

#include "core/ConsoleApp.h"
#include "backend/loop/ServerLoop.h"
#include "core/TimeProvider.h"
#include "network/ServerNetwork.h"

class Server: public core::ConsoleApp {
private:
	using Super = core::ConsoleApp;
	network::ServerNetworkPtr _network;
	backend::ServerLoopPtr _serverLoop;
public:
	Server(const network::ServerNetworkPtr& network, const backend::ServerLoopPtr& serverLoop,
			const core::TimeProviderPtr& timeProvider, const io::FilesystemPtr& filesystem,
			const core::EventBusPtr& eventBus);

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
};
