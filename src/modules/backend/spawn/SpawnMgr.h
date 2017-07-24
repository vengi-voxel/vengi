/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "ServerMessages_generated.h"
#include "ai/common/Types.h"
#include <glm/vec3.hpp>

namespace backend {

class SpawnMgr {
private:
	AILoaderPtr _loader;
	voxel::WorldPtr _world;
	EntityStoragePtr _entityStorage;
	network::MessageSenderPtr _messageSender;
	core::TimeProviderPtr _timeProvider;
	attrib::ContainerProviderPtr _containerProvider;
	poi::PoiProviderPtr _poiProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	long _time;

	void spawnEntity(ai::Zone& zone, network::EntityType start, network::EntityType end, int maxAmount);
	void spawnAnimals(ai::Zone& zone);
	void spawnCharacters(ai::Zone& zone);

public:
	SpawnMgr(const voxel::WorldPtr& world, const EntityStoragePtr& entityStorage, const network::MessageSenderPtr& messageSender,
			const core::TimeProviderPtr& timeProvider, const AILoaderPtr& loader, const attrib::ContainerProviderPtr& containerProvider,
			const poi::PoiProviderPtr& poiProvider, const cooldown::CooldownProviderPtr& cooldownProvider);
	bool init();
	void shutdown();

	int spawn(ai::Zone& zone, network::EntityType type, int amount, const glm::ivec3* pos = nullptr);
	void onFrame(ai::Zone& zone, long dt);
};

typedef std::shared_ptr<SpawnMgr> SpawnMgrPtr;

}
