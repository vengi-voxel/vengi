/**
 * @file
 */

#include "Server.h"
#include "core/Var.h"
#include "core/command/Command.h"
#include "backend/poi/PoiProvider.h"
#include "backend/entity/EntityStorage.h"
#include "backend/entity/ai/AIRegistry.h"
#include "backend/entity/ai/AILoader.h"
#include "backend/loop/ServerLoop.h"
#include "backend/spawn/SpawnMgr.h"

#include <cstdlib>

Server::Server(const network::NetworkPtr& network, const backend::ServerLoopPtr& serverLoop,
		const core::TimeProviderPtr& timeProvider, const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus) :
		core::App(filesystem, eventBus, timeProvider, 15678), _network(network),
		_serverLoop(serverLoop) {
	init(ORGANISATION, "server");
}

core::AppState Server::onConstruct() {
	const core::AppState state = core::App::onConstruct();

	core::Var::get(cfg::DatabaseName, "engine");
	core::Var::get(cfg::DatabaseHost, "localhost");
	core::Var::get(cfg::DatabaseUser, "engine");
	core::Var::get(cfg::DatabasePassword, "engine", core::CV_SECRET);
	core::Var::get(cfg::ServerUserTimeout, "60000");
	core::Var::get(cfg::ServerPort, "11337");
	core::Var::get(cfg::ServerHost, "");
	core::Var::get(cfg::ServerMaxClients, "1024");
	core::Var::get(cfg::ServerAutoRegister, "true");
	core::Var::get(cfg::ServerSeed, "1");
	core::Var::get(cfg::VoxelMeshSize, "128", core::CV_READONLY);
	core::Var::get(cfg::DatabaseMinConnections, "2");
	core::Var::get(cfg::DatabaseMaxConnections, "10");

	return state;
}

core::AppState Server::onInit() {
	const core::AppState state = core::App::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (!_network->init()) {
		Log::error("Failed to init the network");
		return core::AppState::Cleanup;
	}

	if (!_serverLoop->init()) {
		Log::error("Failed to init the main loop");
		return core::AppState::Cleanup;
	}

	const core::VarPtr& port = core::Var::getSafe(cfg::ServerPort);
	const core::VarPtr& host = core::Var::getSafe(cfg::ServerHost);
	const core::VarPtr& maxclients = core::Var::getSafe(cfg::ServerMaxClients);
	if (!_network->bind(port->intVal(), host->strVal(), maxclients->intVal(), 2)) {
		Log::error("Failed to bind the server socket on %s:%i", host->strVal().c_str(), port->intVal());
		return core::AppState::Cleanup;
	}
	Log::info("Server socket is up at %s:%i", host->strVal().c_str(), port->intVal());
	return core::AppState::Running;
}

core::AppState Server::onCleanup() {
	const core::AppState state = core::App::onCleanup();
	_serverLoop->shutdown();
	_network->shutdown();
	return state;
}

core::AppState Server::onRunning() {
	core::App::onRunning();
	_serverLoop->onFrame(_deltaFrame);
	return core::AppState::Running;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const voxel::WorldPtr& world = std::make_shared<voxel::World>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const backend::AIRegistryPtr& registry = std::make_shared<backend::AIRegistry>();
	const attrib::ContainerProviderPtr& containerProvider = std::make_shared<attrib::ContainerProvider>();

	const network::ProtocolHandlerRegistryPtr& protocolHandlerRegistry = std::make_shared<network::ProtocolHandlerRegistry>();
	const network::NetworkPtr& network = std::make_shared<network::Network>(protocolHandlerRegistry, eventBus);
	const network::MessageSenderPtr& messageSender = std::make_shared<network::MessageSender>(network);

	const backend::AILoaderPtr& loader = std::make_shared<backend::AILoader>(registry);

	const cooldown::CooldownProviderPtr& cooldownProvider = std::make_shared<cooldown::CooldownProvider>();

	const backend::PoiProviderPtr& poiProvider = std::make_shared<backend::PoiProvider>(world, timeProvider);
	const backend::EntityStoragePtr& entityStorage = std::make_shared<backend::EntityStorage>(messageSender, world, timeProvider, containerProvider, poiProvider, cooldownProvider);
	const backend::SpawnMgrPtr& spawnMgr = std::make_shared<backend::SpawnMgr>(world, entityStorage, messageSender, timeProvider, loader, containerProvider, poiProvider, cooldownProvider);

	const backend::ServerLoopPtr& serverLoop = std::make_shared<backend::ServerLoop>(network, spawnMgr, world, entityStorage, eventBus, registry, containerProvider, poiProvider, cooldownProvider);

	Server app(network, serverLoop, timeProvider, filesystem, eventBus);
	return app.startMainLoop(argc, argv);
}
