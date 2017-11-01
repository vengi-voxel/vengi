/**
 * @file
 */

#include "World.h"
#include "backend/spawn/SpawnMgr.h"
#include "backend/world/MapProvider.h"
#include "backend/entity/ai/AIRegistry.h"
#include "core/Log.h"
#include <SimpleAI.h>

namespace backend {

constexpr int aiDebugServerPort = 11338;
constexpr const char* aiDebugServerInterface = "127.0.0.1";

World::World(const MapProviderPtr& mapProvider, const SpawnMgrPtr& spawnMgr, const AIRegistryPtr& registry,
		const core::EventBusPtr& eventBus) :
		_spawnMgr(spawnMgr), _mapProvider(mapProvider), _registry(registry), _eventBus(eventBus) {
}

void World::update(long dt) {
	for (auto& e : _maps) {
		MapPtr& map = e.second;
		_spawnMgr->update(map, dt);
		map->update(dt);
	}
	_aiServer->update(dt);
}

bool World::init() {
	_registry->init(_spawnMgr);

	if (!_mapProvider->init()) {
		Log::error("Failed to init the map provider");
		return false;
	}

	if (!_spawnMgr->init()) {
		Log::error("Failed to init the spawn manager");
		return false;
	}

	_aiServer = new ai::Server(*_registry, aiDebugServerPort, aiDebugServerInterface);
	if (_aiServer->start()) {
		Log::info("Start the ai debug server on %s:%i", aiDebugServerInterface, aiDebugServerPort);
	} else {
		Log::error("Could not start the ai debug server");
	}

	_maps = _mapProvider->worldMaps();
	if (_maps.empty()) {
		Log::error("Could not initialize any map");
		return false;
	}
	for (auto& e : _maps) {
		const MapPtr& map = e.second;
		_aiServer->addZone(map->zone());
	}
	return true;
}

void World::shutdown() {
	_spawnMgr->shutdown();
	for (auto& e : _maps) {
		const MapPtr& map = e.second;
		_aiServer->removeZone(map->zone());
	}
	_maps.clear();
	_mapProvider->shutdown();
	delete _aiServer;
	_aiServer = nullptr;
}

}
