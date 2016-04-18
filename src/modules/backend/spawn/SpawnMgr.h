#pragma once

#include <memory>
#include "backend/entity/ai/AICommon.h"
#include "voxel/World.h"
#include "backend/entity/EntityStorage.h"
#include "backend/entity/ai/AILoader.h"
#include "backend/poi/PoiProvider.h"
#include "network/messages/ServerMessages.h"

namespace backend {

class SpawnMgr {
private:
	AILoaderPtr _loader;
	voxel::WorldPtr _world;
	EntityStoragePtr _entityStorage;
	network::MessageSenderPtr _messageSender;
	core::TimeProviderPtr _timeProvider;
	attrib::ContainerProviderPtr _containerProvider;
	PoiProviderPtr _poiProvider;
	long _time;

	void spawnEntity(ai::Zone& zone, network::messages::NpcType start, network::messages::NpcType end, int maxAmount);
	void spawnAnimals(ai::Zone& zone);
	void spawnCharacters(ai::Zone& zone);

public:
	SpawnMgr(voxel::WorldPtr world, EntityStoragePtr entityStorage, network::MessageSenderPtr messageSender, core::TimeProviderPtr timeProvider, AILoaderPtr loader, attrib::ContainerProviderPtr containerProvider, PoiProviderPtr poiProvider);
	bool init();

	int spawn(ai::Zone& zone, network::messages::NpcType type, int amount, const glm::ivec3* pos = nullptr);
	void onFrame(ai::Zone& zone, long dt);
};

typedef std::shared_ptr<SpawnMgr> SpawnMgrPtr;

}
