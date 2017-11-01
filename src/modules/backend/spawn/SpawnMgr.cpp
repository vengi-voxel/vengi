/**
 * @file
 */

#include "SpawnMgr.h"
#include "core/Common.h"
#include "core/Singleton.h"
#include "core/App.h"
#include "io/Filesystem.h"
#include "backend/entity/EntityStorage.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/ai/AILoader.h"
#include "poi/PoiProvider.h"
#include "backend/entity/Npc.h"
#include "backend/world/Map.h"

namespace backend {

static const long spawnTime = 15000L;

SpawnMgr::SpawnMgr(const EntityStoragePtr& entityStorage, const network::ServerMessageSenderPtr& messageSender,
		const core::TimeProviderPtr& timeProvider, const AILoaderPtr& loader, const attrib::ContainerProviderPtr& containerProvider,
		const poi::PoiProviderPtr& poiProvider, const cooldown::CooldownProviderPtr& cooldownProvider) :
		_loader(loader), _entityStorage(entityStorage), _messageSender(messageSender), _timeProvider(timeProvider),
		_containerProvider(containerProvider), _poiProvider(poiProvider), _cooldownProvider(cooldownProvider), _time(15000L) {
}

void SpawnMgr::shutdown() {
}

bool SpawnMgr::init() {
	const io::FilesystemPtr& filesytem = core::App::getInstance()->filesystem();
	const std::string& lua = filesytem->load("behaviourtrees.lua");
	if (!_loader->init(lua)) {
		Log::error("could not load the behaviourtrees: %s", _loader->getError().c_str());
		return false;
	}
	return true;
}

void SpawnMgr::spawnCharacters(const MapPtr& map) {
	spawnEntity(map, network::EntityType::BEGIN_CHARACTERS, network::EntityType::MAX_CHARACTERS, 0);
}

void SpawnMgr::spawnAnimals(const MapPtr& map) {
	spawnEntity(map, network::EntityType::BEGIN_ANIMAL, network::EntityType::MAX_ANIMAL, 2);
}

void SpawnMgr::spawnEntity(const MapPtr& map, network::EntityType start, network::EntityType end, int maxAmount) {
	ai::Zone& zone = *map->zone();
	const int offset = (int)start + 1;
	const int size = (int)end - offset;
	int count[size];
	memset(count, 0, sizeof(count));
	zone.execute([&] (const ai::AIPtr& ai) {
		const AICharacter& chr = ai::character_cast<AICharacter>(ai->getCharacter());
		const Npc& npc = chr.getNpc();
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
		spawn(map, type, needToSpawn);
	}
}

int SpawnMgr::spawn(const MapPtr& map, network::EntityType type, int amount, const glm::ivec3* pos) {
	const char *typeName = network::EnumNameEntityType(type);
	const ai::TreeNodePtr& behaviour = _loader->load(typeName);
	if (!behaviour) {
		Log::error("could not load the behaviour tree %s", typeName);
		return 0;
	}
	for (int x = 0; x < amount; ++x) {
		const NpcPtr& npc = std::make_shared<Npc>(type, behaviour, map, _messageSender,
				_timeProvider, _containerProvider, _cooldownProvider, _poiProvider);
		npc->init(pos);
		// now let it tick
		map->addNpc(npc);
		_entityStorage->addNpc(npc);
	}

	return amount;
}

void SpawnMgr::update(const MapPtr& map, long dt) {
	_time += dt;
	if (_time >= spawnTime) {
		_time -= spawnTime;
		spawnAnimals(map);
		spawnCharacters(map);
	}
}

}
