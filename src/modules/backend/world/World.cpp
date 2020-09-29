/**
 * @file
 */

#include "World.h"
#include "shared/ProtocolEnum.h"
#include "command/Command.h"
#include "backend/spawn/SpawnMgr.h"
#include "backend/world/MapProvider.h"
#include "backend/world/Map.h"
#include "backend/entity/ai/LUAAIRegistry.h"
#include "io/Filesystem.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "LUAFunctions.h"
#include "attrib/ContainerProvider.h"

namespace backend {

World::World(const MapProviderPtr& mapProvider, const AIRegistryPtr& registry,
		const core::EventBusPtr& eventBus, const io::FilesystemPtr& filesystem,
		const metric::MetricPtr& metric) :
		_mapProvider(mapProvider), _registry(registry),
		_eventBus(eventBus), _filesystem(filesystem), _metric(metric) {
}

World::~World() {
	core_assert_msg(_maps.empty(), "World was not properly shut down");
}

void World::update(long dt) {
	core_trace_scoped(WorldUpdate);
	for (const auto& e : _maps) {
		const MapPtr& map = e->value;
		map->update(dt);
	}
	_aiServer->update(dt);
}

void World::construct() {
	command::Command::registerCommand("sv_maplist", [this] (const command::CmdArgs& args) {
		for (const auto& e : _maps) {
			const MapPtr& map = e->value;
			Log::info("Map %s", map->idStr().c_str());
		}
	}).setHelp("List all maps");

	command::Command::registerCommand("sv_spawnnpc", [this] (const command::CmdArgs& args) {
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

	command::Command::registerCommand("sv_chunkstruncate", [this] (const command::CmdArgs& args) {
		const core::VarPtr& seed = core::Var::getSafe(cfg::ServerSeed);
		for (const auto& e : _maps) {
			const MapPtr& map = e->value;
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

	const core::VarPtr& aiDebugServerPort = core::Var::get("aidbg_port", 11338);
	const core::VarPtr& aiDebugServerInterface = core::Var::get("aidbg_host", "127.0.0.1");
	aiDebugServerInterface->setHelp("There is not auth on the debug server.");
	_aiServer = new Server(*_registry, _metric, (short)aiDebugServerPort->intVal(), aiDebugServerInterface->strVal());
	if (_aiServer->start()) {
		Log::info("Start the ai debug server on %s:%i", aiDebugServerInterface->strVal().c_str(), aiDebugServerPort->intVal());
	} else {
		Log::error("Could not start the ai debug server");
	}

	_maps = _mapProvider->worldMaps();
	if (_maps.empty()) {
		Log::error("Could not initialize any map");
		return false;
	}
	for (const auto& e : _maps) {
		const MapPtr& map = e->value;
		_aiServer->addZone(map->zone());
	}

	return true;
}

void World::shutdown() {
	for (const auto& e : _maps) {
		const MapPtr& map = e->value;
		_aiServer->removeZone(map->zone());
	}
	_maps.clear();
	_mapProvider->shutdown();
	delete _aiServer;
	_aiServer = nullptr;
}

}
