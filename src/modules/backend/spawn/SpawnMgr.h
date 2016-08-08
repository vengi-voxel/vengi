/**
 * @file
 */

#pragma once

#include <memory>
#include "voxel/World.h"
#include "backend/entity/EntityStorage.h"
#include "backend/poi/PoiProvider.h"
#include "ServerMessages_generated.h"

namespace backend {

class AILoader;
typedef std::shared_ptr<AILoader> AILoaderPtr;

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

	void spawnEntity(ai::Zone& zone, network::EntityType start, network::EntityType end, int maxAmount);
	void spawnAnimals(ai::Zone& zone);
	void spawnCharacters(ai::Zone& zone);

public:
	SpawnMgr(voxel::WorldPtr world, EntityStoragePtr entityStorage, network::MessageSenderPtr messageSender, core::TimeProviderPtr timeProvider, AILoaderPtr loader, attrib::ContainerProviderPtr containerProvider, PoiProviderPtr poiProvider);
	bool init();
	void shutdown();

	int spawn(ai::Zone& zone, network::EntityType type, int amount, const glm::ivec3* pos = nullptr);
	void onFrame(ai::Zone& zone, long dt);
};

typedef std::shared_ptr<SpawnMgr> SpawnMgrPtr;

}
