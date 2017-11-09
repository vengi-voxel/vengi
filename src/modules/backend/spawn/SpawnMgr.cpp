/**
 * @file
 */

#include "SpawnMgr.h"
#include "core/Common.h"
#include "core/Singleton.h"
#include "io/Filesystem.h"
#include "backend/entity/EntityStorage.h"
#include "backend/entity/ai/AICharacter.h"
#include "backend/entity/ai/AILoader.h"
#include "poi/PoiProvider.h"
#include "backend/entity/Npc.h"
#include "backend/world/Map.h"

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
	spawnEntity(network::EntityType::BEGIN_CHARACTERS, network::EntityType::MAX_CHARACTERS, 0);
}

void SpawnMgr::spawnAnimals() {
	spawnEntity(network::EntityType::BEGIN_ANIMAL, network::EntityType::MAX_ANIMAL, 2);
}

void SpawnMgr::spawnEntity(network::EntityType start, network::EntityType end, int maxAmount) {
	ai::Zone& zone = *_map->zone();
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
		spawn(type, needToSpawn);
	}
}

int SpawnMgr::spawn(network::EntityType type, int amount, const glm::ivec3* pos) {
	const char *typeName = network::EnumNameEntityType(type);
	const ai::TreeNodePtr& behaviour = _loader->load(typeName);
	if (!behaviour) {
		Log::error("could not load the behaviour tree %s", typeName);
		return 0;
	}
	for (int x = 0; x < amount; ++x) {
		const NpcPtr& npc = std::make_shared<Npc>(type, behaviour, _map->ptr(), _messageSender,
				_timeProvider, _containerProvider, _cooldownProvider);
		npc->init(pos);
		// now let it tick
		_map->addNpc(npc);
		_entityStorage->addNpc(npc);
	}

	return amount;
}

void SpawnMgr::update(long dt) {
	_time += dt;
	if (_time >= spawnTime) {
		_time -= spawnTime;
		spawnAnimals();
		spawnCharacters();
	}
}

}
