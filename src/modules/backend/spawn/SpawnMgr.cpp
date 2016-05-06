/**
 * @file
 */

#include "SpawnMgr.h"
#include "core/Common.h"
#include "core/Singleton.h"
#include "core/App.h"
#include "io/Filesystem.h"

namespace backend {

static const long spawnTime = 15000L;

SpawnMgr::SpawnMgr(voxel::WorldPtr world, EntityStoragePtr entityStorage, network::MessageSenderPtr messageSender, core::TimeProviderPtr timeProvider, AILoaderPtr loader, attrib::ContainerProviderPtr containerProvider, PoiProviderPtr poiProvider) :
		_loader(loader), _world(world), _entityStorage(entityStorage), _messageSender(messageSender), _timeProvider(timeProvider), _containerProvider(containerProvider), _poiProvider(poiProvider), _time(15000L) {
}

bool SpawnMgr::init() {
	const std::string& lua = core::App::getInstance()->filesystem()->load("behaviourtrees.lua");
	if (!_loader->init(lua)) {
		Log::error("could not load the behaviourtrees: %s", _loader->getError().c_str());
		return false;
	}
	std::vector<std::string> trees;
	_loader->getTrees(trees);

	Log::info("loaded %i behaviour trees", (int)trees.size());
	for (const std::string& tree : trees) {
		std::stringstream s;
		const ai::TreeNodePtr& node = _loader->load(tree);
		core_assert(node);
		s << *node;
		Log::debug("%s", s.str().c_str());
	}
	return true;
}

void SpawnMgr::spawnCharacters(ai::Zone& zone) {
	spawnEntity(zone, network::messages::NpcType_BEGIN_CHARACTERS, network::messages::NpcType_MAX_CHARACTERS, 0);
}

void SpawnMgr::spawnAnimals(ai::Zone& zone) {
	spawnEntity(zone, network::messages::NpcType_BEGIN_ANIMAL, network::messages::NpcType_MAX_ANIMAL, 2);
}

void SpawnMgr::spawnEntity(ai::Zone& zone, network::messages::NpcType start, network::messages::NpcType end, int maxAmount) {
	const int offset = start + 1;
	int count[end - offset];
	memset(count, 0, sizeof(count));
	zone.execute([start, end, offset, &count] (const ai::AIPtr& ai) {
		const AICharacter& chr = ai::character_cast<AICharacter>(ai->getCharacter());
		const Npc& npc = chr.getNpc();
		const network::messages::NpcType type = npc.npcType();
		if (type <= start || type >= end)
			return;
		const int index = type - offset;
		++count[index];
	});

	const int size = SDL_arraysize(count);
	for (int i = 0; i < size; ++i) {
		if (count[i] >= maxAmount)
			continue;

		const int needToSpawn = maxAmount - count[i];
		network::messages::NpcType type = static_cast<network::messages::NpcType>(offset + i);
		spawn(zone, type, needToSpawn);
	}
}

int SpawnMgr::spawn(ai::Zone& zone, network::messages::NpcType type, int amount, const glm::ivec3* pos) {
	const char *typeName = network::messages::EnumNameNpcType(type);
	ai::TreeNodePtr behaviour = _loader->load(typeName);
	if (!behaviour) {
		Log::error("could not load the behaviour tree %s", typeName);
		return 0;
	}
	for (int x = 0; x < amount; ++x) {
		const NpcPtr& npc = std::make_shared<Npc>(type, _entityStorage, behaviour, _world, _messageSender, _timeProvider, _containerProvider, _poiProvider);
		npc->init(pos);
		// now let it tick
		zone.addAI(npc->ai());
		_entityStorage->addNpc(npc);
	}

	return amount;
}

void SpawnMgr::onFrame(ai::Zone& zone, long dt) {
	_time += dt;
	if (_time >= spawnTime) {
		_time -= spawnTime;
		spawnAnimals(zone);
		spawnCharacters(zone);
	}
}

}
