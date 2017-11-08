/**
 * @file
 */

#pragma once

#include "ServerMessages_generated.h"
#include "backend/ForwardDecl.h"
#include "ai/common/Types.h"
#include <glm/vec3.hpp>

namespace backend {

class SpawnMgr {
private:
	Map* _map;
	AILoaderPtr _loader;
	EntityStoragePtr _entityStorage;
	network::ServerMessageSenderPtr _messageSender;
	core::TimeProviderPtr _timeProvider;
	attrib::ContainerProviderPtr _containerProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	io::FilesystemPtr _filesystem;
	long _time;

	void spawnEntity(network::EntityType start, network::EntityType end, int maxAmount);
	void spawnAnimals();
	void spawnCharacters();

public:
	SpawnMgr(Map* map,
			const io::FilesystemPtr& filesytem,
			const EntityStoragePtr& entityStorage,
			const network::ServerMessageSenderPtr& messageSender,
			const core::TimeProviderPtr& timeProvider,
			const AILoaderPtr& loader,
			const attrib::ContainerProviderPtr& containerProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider);
	bool init();
	void shutdown();

	int spawn(network::EntityType type, int amount, const glm::ivec3* pos = nullptr);
	void update(long dt);
};

typedef std::shared_ptr<SpawnMgr> SpawnMgrPtr;

}
