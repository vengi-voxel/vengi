/**
 * @file
 */

#include "ServerLoop.h"

#include "core/command/Command.h"
#include "core/Log.h"
#include "core/App.h"
#include "io/Filesystem.h"
#include "cooldown/CooldownProvider.h"
#include "persistence/ConnectionPool.h"
#include "DatabaseModels.h"
#include "backend/entity/User.h"
#include "backend/entity/ai/AIRegistry.h"
#include "backend/entity/ai/AICommon.h"
#include "ClientMessages_generated.h"
#include "backend/network/UserConnectHandler.h"
#include "backend/network/UserConnectedHandler.h"
#include "backend/network/UserDisconnectHandler.h"
#include "backend/network/AttackHandler.h"
#include "backend/network/MoveHandler.h"
#include "core/command/CommandHandler.h"
#include "voxel/MaterialColor.h"

namespace backend {

constexpr int aiDebugServerPort = 11338;
constexpr const char* aiDebugServerInterface = "127.0.0.1";

ServerLoop::ServerLoop(const network::ServerNetworkPtr& network, const SpawnMgrPtr& spawnMgr, const voxel::WorldPtr& world, const EntityStoragePtr& entityStorage, const core::EventBusPtr& eventBus, const AIRegistryPtr& registry,
		const attrib::ContainerProviderPtr& containerProvider, const poi::PoiProviderPtr& poiProvider, const cooldown::CooldownProviderPtr& cooldownProvider) :
		_network(network), _spawnMgr(spawnMgr), _world(world),
		_entityStorage(entityStorage), _eventBus(eventBus), _registry(registry), _containerProvider(containerProvider), _poiProvider(poiProvider), _cooldownProvider(cooldownProvider) {
	_world->setClientData(false);
	_eventBus->subscribe<network::NewConnectionEvent>(*this);
	_eventBus->subscribe<network::DisconnectEvent>(*this);
}

#define regHandler(type, handler, ...) \
	r->registerHandler(network::EnumNameClientMsgType(type), std::make_shared<handler>(__VA_ARGS__));

bool ServerLoop::init() {
	if (core::Singleton<::persistence::ConnectionPool>::getInstance().init() <= 0) {
		Log::error("Failed to init the connection pool");
		return false;
	}
	persistence::UserStore u;
	u.createTable();

	if (!_cooldownProvider->init("cooldowns.lua")) {
		Log::error("Failed to load the cooldown configuration: %s", _cooldownProvider->error().c_str());
		return false;
	}

	_zone = new ai::Zone("Zone");
	_aiServer = new ai::Server(*_registry, aiDebugServerPort, aiDebugServerInterface);

	const std::string& attributes = core::App::getInstance()->filesystem()->load("attributes.lua");
	if (!_containerProvider->init(attributes)) {
		Log::error("Failed to load the attributes: %s", _containerProvider->error().c_str());
		return false;
	}
	_registry->init(_spawnMgr);
	if (!_spawnMgr->init()) {
		Log::error("Failed to init the spawn manager");
		return false;
	}

	const network::ProtocolHandlerRegistryPtr& r = _network->registry();
	regHandler(network::ClientMsgType::UserConnect, UserConnectHandler, _network, _entityStorage, _world);
	regHandler(network::ClientMsgType::UserConnected, UserConnectedHandler);
	regHandler(network::ClientMsgType::UserDisconnect, UserDisconnectHandler);
	regHandler(network::ClientMsgType::Attack, AttackHandler);
	regHandler(network::ClientMsgType::Move, MoveHandler);

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return false;
	}

	const io::FilesystemPtr& filesystem = core::App::getInstance()->filesystem();
	if (!_world->init(filesystem->load("world.lua"), filesystem->load("biomes.lua"))) {
		Log::error("Failed to init the world");
		return false;
	}

	const core::VarPtr& seed = core::Var::getSafe(cfg::ServerSeed);
	_world->setSeed(seed->longVal());
	_world->setPersist(false);
	if (_aiServer->start()) {
		Log::info("Start the ai debug server on %s:%i", aiDebugServerInterface, aiDebugServerPort);
		_aiServer->addZone(_zone);
	} else {
		Log::error("Could not start the ai debug server");
	}
	return true;
}

void ServerLoop::shutdown() {
	_world->shutdown();
	core::Singleton<::persistence::ConnectionPool>::getInstance().shutdown();
	_spawnMgr->shutdown();
	delete _zone;
	delete _aiServer;
	_zone = nullptr;
	_aiServer = nullptr;
}

void ServerLoop::readInput() {
	const char *input = _input.read();
	if (input == nullptr) {
		return;
	}

	core::executeCommands(input);
}

void ServerLoop::onFrame(long dt) {
	readInput();
	core_trace_scoped(ServerLoop);
	core::Var::visitReplicate([] (const core::VarPtr& var) {
		Log::info("TODO: %s needs replicate", var->name().c_str());
	});
	_network->update();
	{ // TODO: move into own thread
		core_trace_scoped(PoiUpdate);
		_poiProvider->update(dt);
	}
	{ // TODO: move into own thread
		core_trace_scoped(WorldUpdate);
		_world->onFrame(dt);
	}
	{ // TODO: move into own thread
		core_trace_scoped(AIServerUpdate);
		_zone->update(dt);
		_aiServer->update(dt);
	}
	{ // TODO: move into own thread
		core_trace_scoped(SpawnMgrUpdate);
		_spawnMgr->onFrame(*_zone, dt);
	}
	{
		core_trace_scoped(EntityStorage);
		_entityStorage->onFrame(dt);
	}
}

void ServerLoop::onEvent(const network::DisconnectEvent& event) {
	ENetPeer* peer = event.peer();
	Log::info("disconnect peer: %u", peer->connectID);
	User* user = reinterpret_cast<User*>(peer->data);
	if (user == nullptr) {
		return;
	}
	// TODO: handle this and abort on re-login
	user->cooldownMgr().triggerCooldown(cooldown::Type::LOGOUT);
}

void ServerLoop::onEvent(const network::NewConnectionEvent& event) {
	Log::info("new connection - waiting for login request from %u", event.peer()->connectID);
}

}
