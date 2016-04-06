#include "ServerLoop.h"
#include "core/Command.h"
#include "core/Tokenizer.h"
#include "core/Log.h"

namespace backend {

ServerLoop::ServerLoop(network::NetworkPtr network, SpawnMgrPtr spawnMgr, voxel::WorldPtr world, EntityStoragePtr entityStorage, core::EventBusPtr eventBus, AIRegistryPtr registry,
		attrib::ContainerProviderPtr containerProvider, PoiProviderPtr poiProvider) :
		_network(network), _spawnMgr(spawnMgr), _world(world), _zone("Zone"), _aiServer(*registry, 11338, "127.0.0.1"), _entityStorage(entityStorage), _eventBus(eventBus), _registry(
				registry), _containerProvider(containerProvider), _poiProvider(poiProvider) {
	_world->setClientData(false);
	_eventBus->subscribe<network::NewConnectionEvent>(*this);
	_eventBus->subscribe<network::DisconnectEvent>(*this);
}

bool ServerLoop::onInit() {
	if (!_containerProvider->init()) {
		Log::error("Failed to load the attributes: %s", _containerProvider->error().c_str());
		return false;
	}
	_registry->init(_spawnMgr);
	if (!_spawnMgr->init())
		return false;
	const core::VarPtr& seed = core::Var::get(cfg::ServerSeed, "1");

	_world->setSeed(seed->longVal());
	if (_aiServer.start()) {
		Log::info("Start the ai debug server on 127.0.0.1:11338");
		_aiServer.addZone(&_zone);
	} else {
		Log::error("Could not start the ai debug server");
	}
	return true;
}

void ServerLoop::readInput() {
	const char *input = _input.read();
	if (input == nullptr)
		return;
	if (core::Command::execute(input) != 0)
		return;
	core::Tokenizer t(input);
	while (t.hasNext()) {
		const std::string& var = t.next();
		const core::VarPtr& varPtr = core::Var::get(var, "", core::CV_NOTCREATEEMPTY);
		if (!varPtr)
			break;
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
	core_trace_scoped("ServerLoop");
	_network->update();
	{ // TODO: move into own thread
		core_trace_scoped("PoiUpdate");
		_poiProvider->update(dt);
	}
	{ // TODO: move into own thread
		core_trace_scoped("WorldUpdate");
		_world->onFrame(dt);
	}
	{ // TODO: move into own thread
		core_trace_scoped("AIServerUpdate");
		_zone.update(dt);
		_aiServer.update(dt);
	}
	{ // TODO: move into own thread
		core_trace_scoped("SpawnMgrUpdate");
		_spawnMgr->onFrame(_zone, dt);
	}
	{
		core_trace_scoped("EntityStorage");
		_entityStorage->onFrame(dt);
	}
}

void ServerLoop::onEvent(const network::DisconnectEvent& event) {
	Log::info("disconnect peer: %i", event.peer()->connectID);
}

void ServerLoop::onEvent(const network::NewConnectionEvent& event) {
	Log::info("new connection - waiting for login request from %i", event.peer()->connectID);
}

}
