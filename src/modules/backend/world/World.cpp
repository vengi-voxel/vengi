/**
 * @file
 */

#include "World.h"
#include "network/ProtocolEnum.h"
#include "core/command/Command.h"
#include "backend/spawn/SpawnMgr.h"
#include "backend/world/MapProvider.h"
#include "backend/world/Map.h"
#include "backend/entity/ai/LUAAIRegistry.h"
#include "core/io/Filesystem.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "LUAFunctions.h"
#include "attrib/ContainerProvider.h"

namespace backend {

constexpr int aiDebugServerPort = 11338;
constexpr const char* aiDebugServerInterface = "127.0.0.1";

World::World(const MapProviderPtr& mapProvider, const AIRegistryPtr& registry,
		const core::EventBusPtr& eventBus, const io::FilesystemPtr& filesystem) :
		_mapProvider(mapProvider), _registry(registry),
		_eventBus(eventBus), _filesystem(filesystem) {
}

World::~World() {
	core_assert_msg(_maps.empty(), "World was not properly shut down");
}

void World::update(long dt) {
	core_trace_scoped(WorldUpdate);
	for (auto& e : _maps) {
		const MapPtr& map = e.second;
		map->update(dt);
	}
	_aiServer->update(dt);
}

void World::construct() {
	core::Command::registerCommand("sv_maplist", [this] (const core::CmdArgs& args) {
		for (auto& e : _maps) {
			const MapPtr& map = e.second;
			Log::info("Map %s", map->idStr().c_str());
		}
	}).setHelp("List all maps");

	core::Command::registerCommand("sv_spawnnpc", [this] (const core::CmdArgs& args) {
		if (args.size() < 2) {
			Log::info("Usage: sv_spawnnpc <mapid> <npctype> [amount:default=1]");
			Log::info("entity types are:");
			for (const char *const *t = network::EnumNamesEntityType(); *t != nullptr; ++t) {
				Log::info(" - %s", *t);
			}
			return;
		}
		const MapId id = core::string::toInt(args[0]);
		auto i = _maps.find(id);
		if (i == _maps.end()) {
			Log::info("Could not find the specified map");
			return;
		}
		const MapPtr& map = i->second;
		auto type = network::getEnum<network::EntityType>(args[1].c_str(), network::EnumNamesEntityType());
		if (type == network::EntityType::NONE) {
			Log::error("Invalid entity type given");
			return;
		}
		const int amount = args.size() == 3 ? core::string::toInt(args[2]) : 1;
		map->spawnMgr().spawn((network::EntityType)type, amount);
	}).setHelp("Spawns a given amount of npcs of a particular type on the specified map");

	core::Command::registerCommand("sv_chunkstruncate", [this] (const core::CmdArgs& args) {
		const core::VarPtr& seed = core::Var::getSafe(cfg::ServerSeed);
		for (auto& e : _maps) {
			const MapPtr& map = e.second;
			Log::info("Truncate chunks on map %i for seed %u", map->id(), seed->uintVal());
			map->chunkPersister()->truncate(seed->uintVal());
		}
	}).setHelp("Truncate chunks for all maps");

	_mapProvider->construct();
}

bool World::init() {
	if (!_registry->init()) {
		Log::error("Failed to init the ai registry");
		return false;
	}

	if (!_mapProvider->init()) {
		Log::error("Failed to init the map provider");
		return false;
	}

	_aiServer = new Server(*_registry, aiDebugServerPort, aiDebugServerInterface);
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
