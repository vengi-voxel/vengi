/**
 * @file
 */

#include "SpawnMgr.h"
#include "core/Common.h"
#include "core/Singleton.h"
#include "core/Trace.h"
#include "io/Filesystem.h"
#include "backend/entity/EntityStorage.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/AILoader.h"
#include "backend/entity/ai/zone/Zone.h"
#include "poi/PoiProvider.h"
#include "backend/entity/Npc.h"
#include "backend/world/Map.h"
#include "attrib/ContainerProvider.h"

namespace backend {

static const long spawnTime = 15000L;

SpawnMgr::SpawnMgr(Map* map,
		const io::FilesystemPtr& filesytem,
		const EntityStoragePtr& entityStorage,
		const network::ServerMessageSenderPtr& messageSender,
		const core::TimeProviderPtr& timeProvider,
		const AILoaderPtr& loader,
		const attrib::ContainerProviderPtr& containerProvider,
		const cooldown::CooldownProviderPtr& cooldownProvider) :
		_map(map), _loader(loader), _entityStorage(entityStorage), _messageSender(messageSender), _timeProvider(timeProvider),
		_containerProvider(containerProvider), _cooldownProvider(cooldownProvider),
		_filesystem(filesytem) {
}

void SpawnMgr::shutdown() {
}

bool SpawnMgr::init() {
	return true;
}

void SpawnMgr::spawnCharacters() {
	// TODO: let this number come from the map lua script
	spawnEntity(network::EntityType::BEGIN_CHARACTERS, network::EntityType::MAX_CHARACTERS, 1);
}

void SpawnMgr::spawnAnimals() {
	// TODO: let this number come from the map lua script
	spawnEntity(network::EntityType::BEGIN_ANIMAL, network::EntityType::MAX_ANIMAL, 1);
}

void SpawnMgr::spawnEntity(network::EntityType start, network::EntityType end, int maxAmount) {
	Zone& zone = *_map->zone();
	const int offset = (int)start + 1;
	const int size = (int)end - offset;
	std::vector<int> count(size, 0);
	zone.execute([&] (const AIPtr& ai) {
		Npc& npc = getNpc(ai);
		const network::EntityType type = npc.entityType();
		if (type <= start || type >= end) {
			return;
		}
		const int index = (int)type - offset;
		++count[index];
	});

	for (int i = 0; i < size; ++i) {
		if (count[i] >= maxAmount) {
			continue;
		}

		const int needToSpawn = maxAmount - count[i];
		network::EntityType type = static_cast<network::EntityType>(offset + i);
		spawn(type, needToSpawn);
	}
}

bool SpawnMgr::onSpawn(const NpcPtr& npc, const glm::ivec3* pos) {
	npc->init(pos);
	// now let it tick
	if (_map->addNpc(npc)) {
		_entityStorage->addNpc(npc);
		return true;
	}
	return false;
}

NpcPtr SpawnMgr::createNpc(network::EntityType type, const TreeNodePtr& behaviour) {
	return std::make_shared<Npc>(type, behaviour, _map->ptr(), _messageSender,
					_timeProvider, _containerProvider, _cooldownProvider);
}

NpcPtr SpawnMgr::spawn(network::EntityType type, const glm::ivec3* pos) {
	const char *typeName = network::EnumNameEntityType(type);
	const TreeNodePtr& behaviour = _loader->load(typeName);
	if (!behaviour) {
		Log::error("could not load the behaviour tree %s", typeName);
		return NpcPtr();
	}
	const NpcPtr& npc = createNpc(type, behaviour);
	if (!onSpawn(npc, pos)) {
		return NpcPtr();
	}
	return npc;
}

int SpawnMgr::spawn(network::EntityType type, int amount, const glm::ivec3* pos) {
	const bool isAnimal = core::enumVal(type) > core::enumVal(network::EntityType::BEGIN_ANIMAL) && core::enumVal(type) < core::enumVal(network::EntityType::MAX_ANIMAL);
	const bool isCharacter = core::enumVal(type) > core::enumVal(network::EntityType::BEGIN_CHARACTERS) && core::enumVal(type) < core::enumVal(network::EntityType::MAX_CHARACTERS);
	if (!isAnimal && !isCharacter) {
		Log::error("Currently only animals and characters are supported here");
		return 0;
	}

	const char *typeName = network::EnumNameEntityType(type);
	const TreeNodePtr& behaviour = _loader->load(typeName);
	if (!behaviour) {
		Log::error("could not load the behaviour tree %s", typeName);
		return 0;
	}
	for (int x = 0; x < amount; ++x) {
		const NpcPtr& npc = createNpc(type, behaviour);
		onSpawn(npc, pos);
	}

	return amount;
}

void SpawnMgr::update(long dt) {
	core_trace_scoped(SpawnMgrUpdate);
	_time += dt;
	if (_time >= spawnTime) {
		_time -= spawnTime;
		spawnAnimals();
		spawnCharacters();
	}
}

}
