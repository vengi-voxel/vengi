/**
 * @file
 */

#include "ServerLoop.h"
#include "core/Command.h"
#include "core/Tokenizer.h"
#include "core/Log.h"
#include "cooldown/CooldownDuration.h"
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

namespace backend {

constexpr int aiDebugServerPort = 11338;
constexpr const char* aiDebugServerInterface = "127.0.0.1";

ServerLoop::ServerLoop(const network::NetworkPtr& network, const SpawnMgrPtr& spawnMgr, const voxel::WorldPtr& world, const EntityStoragePtr& entityStorage, const core::EventBusPtr& eventBus, const AIRegistryPtr& registry,
		const attrib::ContainerProviderPtr& containerProvider, const PoiProviderPtr& poiProvider, const cooldown::CooldownDurationPtr& cooldownDuration) :
		_network(network), _spawnMgr(spawnMgr), _world(world), _zone("Zone"), _aiServer(*registry, aiDebugServerPort, aiDebugServerInterface),
		_entityStorage(entityStorage), _eventBus(eventBus), _registry(registry), _containerProvider(containerProvider), _poiProvider(poiProvider), _cooldownDuration(cooldownDuration) {
	_world->setClientData(false);
	_eventBus->subscribe<network::NewConnectionEvent>(*this);
	_eventBus->subscribe<network::DisconnectEvent>(*this);
}

bool ServerLoop::init() {
	if (core::Singleton<::persistence::ConnectionPool>::getInstance().init() <= 0) {
		Log::error("Failed to init the connection pool");
		return false;
	}
	persistence::UserStore u;
	u.createTable();

	if (!_cooldownDuration->init("cooldowns.lua")) {
		Log::error("Failed to load the cooldown configuration: %s", _cooldownDuration->error().c_str());
		return false;
	}

	if (!_containerProvider->init("attributes.lua")) {
		Log::error("Failed to load the attributes: %s", _containerProvider->error().c_str());
		return false;
	}
	_registry->init(_spawnMgr);
	if (!_spawnMgr->init()) {
		Log::error("Failed to init the spawn manager");
		return false;
	}

	const network::ProtocolHandlerRegistryPtr& r = _network->registry();
	r->registerHandler(network::EnumNameClientMsgType(network::ClientMsgType::UserConnect), std::make_shared<UserConnectHandler>(_network, _entityStorage, _world));
	r->registerHandler(network::EnumNameClientMsgType(network::ClientMsgType::UserConnected), std::make_shared<UserConnectedHandler>());
	r->registerHandler(network::EnumNameClientMsgType(network::ClientMsgType::UserDisconnect), std::make_shared<UserDisconnectHandler>());
	r->registerHandler(network::EnumNameClientMsgType(network::ClientMsgType::Attack), std::make_shared<AttackHandler>());
	r->registerHandler(network::EnumNameClientMsgType(network::ClientMsgType::Move), std::make_shared<MoveHandler>());

	const core::VarPtr& seed = core::Var::get(cfg::ServerSeed, "1");
	_world->setSeed(seed->longVal());
	if (_aiServer.start()) {
		Log::info("Start the ai debug server on %s:%i", aiDebugServerInterface, aiDebugServerPort);
		_aiServer.addZone(&_zone);
	} else {
		Log::error("Could not start the ai debug server");
	}
	return true;
}

void ServerLoop::shutdown() {
	_world->shutdown();
	core::Singleton<::persistence::ConnectionPool>::getInstance().shutdown();
	_spawnMgr->shutdown();
}

void ServerLoop::readInput() {
	const char *input = _input.read();
	if (input == nullptr) {
		return;
	}
	if (core::Command::execute(input) != 0) {
		return;
	}
	core::Tokenizer t(input);
	while (t.hasNext()) {
		const std::string& var = t.next();
		const core::VarPtr& varPtr = core::Var::get(var);
		if (!varPtr) {
			break;
		}
		if (!t.hasNext()) {
			if (varPtr) {
				Log::info("%s = %s", varPtr->name().c_str(), varPtr->strVal().c_str());
			} else {
				Log::error("unknown command");
			}
			break;
		}
		const std::string& value = t.next();
		varPtr->setVal(value);
	}
}

void ServerLoop::onFrame(long dt) {
	readInput();
	core_trace_scoped(ServerLoop);
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
		_zone.update(dt);
		_aiServer.update(dt);
	}
	{ // TODO: move into own thread
		core_trace_scoped(SpawnMgrUpdate);
		_spawnMgr->onFrame(_zone, dt);
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
